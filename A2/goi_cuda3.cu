#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include "settings.h"

extern "C" {
#include "goi_cuda.h"
}

#define LINUX
long long wall_clock_time()
{
#ifdef LINUX
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return (long long)(tp.tv_nsec + (long long)tp.tv_sec * 1000000000ll);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_usec * 1000 + (long long)tv.tv_sec * 1000000000ll);
#endif
}

// including the "dead faction": 0
#define MAX_FACTIONS 10

// this macro is here to make the code slightly more readable, not because it can be safely changed to
// any integer value; changing this to a non-zero value may break the code
#define DEAD_FACTION 0

void check_cuda_errors()
{
    cudaError_t rc;
    rc = cudaGetLastError();
    if (rc != cudaSuccess)
    {
        printf("Last CUDA error %s\n", cudaGetErrorString(rc));
    }

}

/**
 * Specifies the number(s) of live neighbors of the same faction required for a dead cell to become alive.
 */
 __host__ __device__ bool isBirthable(int n)
{
    return n == 3;
}

/**
 * Specifies the number(s) of live neighbors of the same faction required for a live cell to remain alive.
 */
 __host__ __device__ bool isSurvivable(int n)
{
    return n == 2 || n == 3;
}

/**
 * Specifies the number of live neighbors of a different faction required for a live cell to die due to fighting.
 */
 __host__ __device__ bool willFight(int n) {
    return n > 0;
}

/**
 * returns the value at the input row and col of the input grid, if valid.
 * 
 * -1 is returned if row or col is out of bounds (as specified by nRows and nCols).
 */
 __host__ __device__ int getValueAt(const int *grid, int nRows, int nCols, int row, int col)
{
    if (row < 0 || row >= nRows || col < 0 || col >= nCols)
    {
        return -1;
    }

    return *(grid + (row * nCols) + col);
}

/**
 * sets the value at the input row and col of the input grid to val.
 * 
 * Does nothing if row or col is out of bounds (as specified by nRows and nCols).
 */
 
 __host__ __device__ void setValueAt(int *grid, int nRows, int nCols, int row, int col, int val)
{
    if (row < 0 || row >= nRows || col < 0 || col >= nCols)
    {
        return;
    }

    *(grid + (row * nCols) + col) = val;
}

/**
 * Computes and returns the next state of the cell specified by row and col based on currWorld and invaders. Sets *diedDueToFighting to
 * true if this cell should count towards the death toll due to fighting.
 * 
 * invaders can be NULL if there are no invaders.
 */
__host__ __device__ int getNextState(const int *currWorld, const int *invaders, int nRows, int nCols, int row, int col, bool *diedDueToFighting)
{
    // we'll explicitly set if it was death due to fighting
    *diedDueToFighting = false;

    // faction of this cell
    int cellFaction = getValueAt(currWorld, nRows, nCols, row, col);

    // did someone just get landed on?
    if (invaders != NULL && getValueAt(invaders, nRows, nCols, row, col) != DEAD_FACTION)
    {   
        *diedDueToFighting = cellFaction != DEAD_FACTION;
        return getValueAt(invaders, nRows, nCols, row, col);
    }

    // tracks count of each faction adjacent to this cell
    int neighborCounts[MAX_FACTIONS];
    memset(neighborCounts, 0, MAX_FACTIONS * sizeof(int));

    // count neighbors (and self)
    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            int faction = getValueAt(currWorld, nRows, nCols, row + dy, col + dx);
            if (faction >= DEAD_FACTION)
            {
                neighborCounts[faction]++;
            }
        }
    }

    // we counted this cell as its "neighbor"; adjust for this
    neighborCounts[cellFaction]--;

    if (cellFaction == DEAD_FACTION)
    {
        // this is a dead cell; we need to see if a birth is possible:
        // need exactly 3 of a single faction; we don't care about other factions

        // by default, no birth
        int newFaction = DEAD_FACTION;

        // start at 1 because we ignore dead neighbors
        for (int faction = DEAD_FACTION + 1; faction < MAX_FACTIONS; faction++)
        {
            int count = neighborCounts[faction];
            if (isBirthable(count))
            {
                newFaction = faction;
            }
        }

        return newFaction;
    }
    else
    {
        /** 
         * this is a live cell; we follow the usual rules:
         * Death (fighting): > 0 hostile neighbor
         * Death (underpopulation): < 2 friendly neighbors and 0 hostile neighbors
         * Death (overpopulation): > 3 friendly neighbors and 0 hostile neighbors
         * Survival: 2 or 3 friendly neighbors and 0 hostile neighbors
         */

        int hostileCount = 0;
        for (int faction = DEAD_FACTION + 1; faction < MAX_FACTIONS; faction++)
        {
            if (faction == cellFaction)
            {
                continue;
            }
            hostileCount += neighborCounts[faction];
        }

        if (willFight(hostileCount))
        {
            *diedDueToFighting = true;
            return DEAD_FACTION;
        }

        int friendlyCount = neighborCounts[cellFaction];
        if (!isSurvivable(friendlyCount))
        {
            return DEAD_FACTION;
        }

        return cellFaction;
    }
}

__device__ __managed__ int deathToll = 0;
//__device__ __managed__ int counter = 0;


__global__ void updateState(int nRows, int nCols, int *world, int *inv, int *wholeNewWorld, int nTask, int nThread, int taskPerBlock, int* row_start, int* col_start, int extra)
{
    //atomicAdd(&counter, 1);
    //printf("counter: %d\n", counter);
    int blkid = blockIdx.x + blockIdx.y * gridDim.x + gridDim.x * gridDim.y * blockIdx.z;
    int tid = blkid * (blockDim.x*blockDim.y*blockDim.z) + (threadIdx.z*(blockDim.x*blockDim.y))
    + (threadIdx.y*blockDim.x) + threadIdx.x;
    
    //printf("nTask: %d, nThread: %d \n", nTask, nThread);
    
    if(tid < nTask)
    {

        if(tid >= nThread - extra)
        {
            taskPerBlock += 1;
        }
       
        int row_counter = 0;
        //int row_start = start[tid] / nCols;
        //int col_start = start[tid] % nCols;
        for(int i = 0; i < taskPerBlock; i ++)
        {
            
            int row = row_start[tid] + row_counter;
            int col = (col_start[tid] + i) % nCols;
            if(col == nCols - 1)
            {
                row_counter += 1;
            }

            /*
            if(tid == 31)
            {
                printf("tid: %d, row: %d, col: %d\n", tid,row, col);
            }*/

            bool diedDueToFighting;
            int nextState = getNextState(world, inv, nRows, nCols, row, col, &diedDueToFighting); //read row * col
            setValueAt(wholeNewWorld, nRows, nCols, row, col, nextState); //write row * col
            if (diedDueToFighting)
            {
                atomicAdd(&deathToll, 1); //!global, mutex
                if(deathToll%10000 == 0)
                {
                    printf("deathToll added: %d\n", deathToll);
                }
            }
        }
    }
    else
    {
        return;
    }
}

/**
 * The main simulation logic.
 * 
 * goi does not own startWorld, invasionTimes or invasionPlans and should not modify or attempt to free them.
 * nThreads is the number of threads to simulate with. It is ignored by the sequential implementation.
 */
int goi(int gridX, int gridY, int gridZ, int blockX, int blockY, int blockZ, int nGenerations, const int *startWorld, int nRows, int nCols, int nInvasions, const int *invasionTimes, int **invasionPlans)
{
    //!CLOCK
    long long before, after;
    before = wall_clock_time();
    //!!!!
    // death toll due to fighting
    //int deathToll = 0; //!!!!change to global
    deathToll = 0;
    
    int nThread = gridX * gridY * gridZ * blockX * blockY * blockZ;
    int nTask = nRows * nCols;

    if(nTask < nThread)
    {
        nThread = nTask;
    }

    int taskPerBlock = nTask / nThread;
    int extra = nTask % nThread;

    int *row_start;
    cudaMallocManaged((void**)&row_start, sizeof(int) * nThread);
    int *col_start;
    cudaMallocManaged((void**)&col_start, sizeof(int) * nThread);
    
    for(int i = 0; i < nThread - extra; i ++)
    {
        row_start[i] = i*taskPerBlock / nCols;
        col_start[i] = i*taskPerBlock % nCols;
    }
    
    for(int i = nThread - extra; i < nThread; i ++)
    {
        row_start[i] = ((nThread - extra) * taskPerBlock + (i - (nThread - extra))*(taskPerBlock + 1)) / nCols;
        col_start[i] = ((nThread - extra) * taskPerBlock + (i - (nThread - extra))*(taskPerBlock + 1)) % nCols;
    }



    dim3 dimGrd(gridX, gridY, gridZ);
    dim3 dimBlk(blockX, blockY, blockZ);
    
    // init the world!
    // we make a copy because we do not own startWorld (and will perform free() on world)
    //int *world = malloc(sizeof(int) * nRows * nCols); 
    
    //!!!cudaMalloc
    int *world = NULL;
    cudaMallocManaged((void **)&world, sizeof(int) * nRows * nCols);
    if (world == NULL)
    {
        cudaFree(world);
        return -1;
    }
    //!!!

    for (int row = 0; row < nRows; row++)
    {
        for (int col = 0; col < nCols; col++)
        {
            setValueAt(world, nRows, nCols, row, col, getValueAt(startWorld, nRows, nCols, row, col));
        }
    }
   
    // Begin simulating
    int invasionIndex = 0;
    for (int i = 1; i <= nGenerations; i++)
    {
        // is there an invasion this generation?
        int *inv = NULL;
        if (invasionIndex < nInvasions && i == invasionTimes[invasionIndex])
        {
            // we make a copy because we do not own invasionPlans
            //inv = malloc(sizeof(int) * nRows * nCols);
            //!cudaMalloc
            cudaMallocManaged((void **)&inv, sizeof(int) * nRows * nCols);
            if (inv == NULL)
            {
                cudaFree(inv);
                return -1;
            }
            //!!!
            
            for (int row = 0; row < nRows; row++)
            {
                for (int col = 0; col < nCols; col++)
                {
                    setValueAt(inv, nRows, nCols, row, col, getValueAt(invasionPlans[invasionIndex], nRows, nCols, row, col));
                }
            }
            invasionIndex++;
        }

        // create the next world state
        //int *wholeNewWorld = malloc(sizeof(int) * nRows * nCols); 
        //!!!cudaMalloc
        int *wholeNewWorld = NULL;
        cudaMallocManaged((void **)&wholeNewWorld, sizeof(int) * nRows * nCols);

        if (wholeNewWorld == NULL)
        {
            if (inv != NULL)
            {
                cudaFree(inv);
            }
            cudaFree(world);
            return -1;
        }
        //!!!!

        //************************************************CUDA CODE************************************************
        //TODO: call kernel
        updateState<<<dimGrd, dimBlk>>>(nRows, nCols, world, inv, wholeNewWorld, nTask, nThread, taskPerBlock, row_start, col_start, extra);
        check_cuda_errors();
        cudaDeviceSynchronize();
        //printf("counter: %d\n", counter);

        //*********************************************************************************************************

        if (inv != NULL)
        {
            cudaFree(inv);
        }

        // swap worlds
        cudaFree(world);
        world = wholeNewWorld;

    }

    //printf("counter: %d\n", counter);


    //!clock end
    after = wall_clock_time();
    fprintf(stderr, "Operation took %1.2f seconds\n", ((float)(after - before)) / 1000000000);
    //!

    return deathToll;
}
