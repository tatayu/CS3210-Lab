#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include "util.h"
#include "exporter.h"
#include "settings.h"

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

/**
 * Specifies the number(s) of live neighbors of the same faction required for a dead cell to become alive.
 */
bool isBirthable(int n)
{
    return n == 3;
}

/**
 * Specifies the number(s) of live neighbors of the same faction required for a live cell to remain alive.
 */
bool isSurvivable(int n)
{
    return n == 2 || n == 3;
}

/**
 * Specifies the number of live neighbors of a different faction required for a live cell to die due to fighting.
 */
bool willFight(int n) {
    return n > 0;
}

/**
 * Computes and returns the next state of the cell specified by row and col based on currWorld and invaders. Sets *diedDueToFighting to
 * true if this cell should count towards the death toll due to fighting.
 * 
 * invaders can be NULL if there are no invaders.
 */
int getNextState(const int *currWorld, const int *invaders, int nRows, int nCols, int row, int col, bool *diedDueToFighting)
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

pthread_mutex_t deathTollMux;

int g_nRows = 0;
int g_nCols = 0;
int *g_world;
const int *g_startWorld;
int *g_inv;
int **g_invasionPlans;
int g_invasionIndex = 0;
int *g_wholeNewWorld;
int g_deathToll = 0;
int *g_upper, *g_lower;

//!functions!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void* copyWorld(void* threadid)
{
    for (int row = g_lower[*(int*)threadid]; row < g_upper[*(int*)threadid]; row++)
    {
        //printf("copyWorld: thread id value is %d, row number is %d\n", *(int*)threadid, row);
        for (int col = 0; col < g_nCols; col++)
        {
            setValueAt(g_world, g_nRows, g_nCols, row, col, getValueAt(g_startWorld, g_nRows, g_nCols, row, col));
        }
    }
}

void* copyInvasionWorld(void* threadid)
{
    for (int row = g_lower[*(int*)threadid]; row < g_upper[*(int*)threadid]; row++)
    {
        //printf("copyInvasionWorld: thread id value is %d, row number is %d\n", *(int*)threadid, row);
        for (int col = 0; col < g_nCols; col++)
        {
            setValueAt(g_inv, g_nRows, g_nCols, row, col, getValueAt(g_invasionPlans[g_invasionIndex], g_nRows, g_nCols, row, col));
        }
    }
}


void* getNewState(void* threadid)
{
    for (int row = g_lower[*(int*)threadid]; row < g_upper[*(int*)threadid]; row++)
    {
        //printf("getNewState: thread id value is %d, row number is %d\n", *(int*)threadid, row);
        for (int col = 0; col < g_nCols; col++)
        {
            bool diedDueToFighting;
            int nextState = getNextState(g_world, g_inv, g_nRows, g_nCols, row, col, &diedDueToFighting);
            setValueAt(g_wholeNewWorld, g_nRows, g_nCols, row, col, nextState);
            if (diedDueToFighting)
            {
                //!mux
                pthread_mutex_lock(&deathTollMux);
                g_deathToll++;
                pthread_mutex_unlock(&deathTollMux);
            }
        }
    }
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


/**
 * The main simulation logic.
 * 
 * goi does not own startWorld, invasionTimes or invasionPlans and should not modify or attempt to free them.
 * nThreads is the number of threads to simulate with. It is ignored by the sequential implementation.
 */
int goi(int nThreads, int nGenerations, const int *startWorld, int nRows, int nCols, int nInvasions, const int *invasionTimes, int **invasionPlans)
{
    //!CLOCK
    long long before, after;
    before = wall_clock_time();
    //!!!!
    int threads_count;
    int rows;

    if(nRows < nThreads)
    {
        threads_count = nRows;
    }
    else
    {
        threads_count = nThreads;
    }

    pthread_t threads[threads_count];

    g_nRows = nRows;
    g_nCols = nCols;
    g_startWorld = startWorld;
    g_invasionPlans = invasionPlans;
    
    int threadid[threads_count];

    // init the world!
    // we make a copy because we do not own startWorld (and will perform free() on world)
    g_world = malloc(sizeof(int) * nRows * nCols);
    if (g_world == NULL)
    {
        return -1;
    }

    //! distribute tasks to threads 
    g_lower = malloc(sizeof(int) * threads_count);
    g_upper = malloc(sizeof(int) * threads_count);
    int average = nRows/threads_count;
    if(nRows % threads_count != 0)
    {
        average += 1;
    }

    int extra_task = average * threads_count - nRows;

    for(int i = 0; i < threads_count - extra_task; i ++)
    {
        g_lower[i] = i * average;
        g_upper[i] = g_lower[i] + average;
    }

    for(int i = threads_count - extra_task; i < threads_count; i ++)
    {
        g_lower[i] = g_upper[i - 1];
        g_upper[i] = g_lower[i] + average - 1;
    }
    
    //! parallel copyWorld
    int rc;
    int t_num;
    for (t_num = 0; t_num < threads_count; t_num++)
    {
        int tid = t_num;
        threadid[tid] = tid;
        rc = pthread_create(&threads[tid], NULL, copyWorld, (void*) &threadid[tid]);
        if (rc) {
            printf("Error: Return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    //wait for all thread to be back
    for (t_num = 0; t_num < threads_count; t_num++) 
    {
        pthread_join(threads[t_num], NULL);
    }


#if PRINT_GENERATIONS
    printf("\n=== WORLD 0 ===\n");
    printWorld(g_world, nRows, nCols);
#endif

#if EXPORT_GENERATIONS
    exportWorld(g_world, nRows, nCols);
#endif

    // Begin simulating
    for (int i = 1; i <= nGenerations; i++)
    {
        // is there an invasion this generation?
        g_inv = NULL;
        if (g_invasionIndex < nInvasions && i == invasionTimes[g_invasionIndex])
        {
            // we make a copy because we do not own invasionPlans
            g_inv = malloc(sizeof(int) * nRows * nCols);
            if (g_inv == NULL)
            {
                free(g_world);
                return -1;
            }

            //! Parallel copyinv
            for (t_num = 0; t_num < threads_count; t_num++)
            {
                int tid = t_num;
                threadid[tid] = tid;
                rc = pthread_create(&threads[t_num], NULL, copyInvasionWorld, (void*) &threadid[tid]);
                if (rc) {
                    printf("Error: Return code from pthread_create() is %d\n", rc);
                    exit(-1);
                }
            }

            for (t_num = 0; t_num < threads_count; t_num++) 
            {
                pthread_join(threads[t_num], NULL);
            }
            g_invasionIndex++;
        }

        // create the next world state
        g_wholeNewWorld = malloc(sizeof(int) * nRows * nCols);
        if (g_wholeNewWorld == NULL)
        {
            if (g_inv != NULL)
            {
                free(g_inv);
            }
            free(g_world);
            return -1;
        }

        // get new states for each cell
        //! parallel get new state
        for (t_num = 0; t_num < threads_count; t_num++)
        {
            int tid = t_num;
            threadid[tid] = tid;
            rc = pthread_create(&threads[t_num], NULL, getNewState, (void*) &threadid[tid]);
            if (rc) {
                printf("Error: Return code from pthread_create() is %d\n", rc);
                exit(-1);
            }
        }

        for (t_num = 0; t_num < threads_count; t_num++) 
        {
            pthread_join(threads[t_num], NULL);
        }

        if (g_inv != NULL)
        {
            free(g_inv);
        }

        // swap worlds
        free(g_world);
        g_world = g_wholeNewWorld;

#if PRINT_GENERATIONS
        printf("\n=== WORLD %d ===\n", i);
        printWorld(world, nRows, nCols);
#endif

#if EXPORT_GENERATIONS
        exportWorld(world, nRows, nCols);
#endif
    }

    free(g_world);
    free(g_lower);
    free(g_upper);

    //!clean up
    pthread_mutex_destroy(&deathTollMux);

    //!clock end
    after = wall_clock_time();
    fprintf(stderr, "Operation took %1.2f seconds\n", ((float)(after - before)) / 1000000000);
    //!

    return g_deathToll;
}
