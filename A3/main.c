#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "tasks.h"
#include "utils.h"
#include <string.h>

int MAX = 20000000; //~20MB

char *get_file_content(char *input_files_dir, int i)
{
    char *filepath = input_files_dir;
    char *slash = "/";
    char *txt = ".txt";
    strcat(filepath, slash);

    char *num;
    sprintf(num, "%d", i);
    strcat(filepath, num);
    printf("%s\n", filepath);
    char *filename = strcat(filepath, txt);
    FILE *fp = fopen(filename, "r");

    if(fp == NULL)
    {
        printf("Error: could not open file %s", filename);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    char *file_content = (char*)malloc(size);

    rewind(fp);
    fread((void*)file_content, 1, size, fp);
    
    return file_content;
}


void master(char *input_files_dir, int num_files)
{
    for(int i = 0; i < num_files; i ++)
    {
        //Step 1. Read the files into buffer
        char *file_content = get_file_content(input_files_dir, i);

        //Step 2. (map) send the data to the map worker
        if(i < num_map_workers)
        {
            rc = MPI_Send(file_content, strlen(file_content), MPI_CHAR, i + 1, i + 1, MPI_COMM_WORLD);
        }
        else
        {
            //wait for idle worker then send the rest of the files
            printf("receiving from any completed worker!")
            MapTaskOutput *output = (MapTaskOutput *)malloc(sizeof(MapTaskOutput));
            MPI_Recv(output, MAX, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, Stat);
            MPI_Send(file_content, strlen(file_content), MPI_CHAR, i + 1, i + 1, MPI_COMM_WORLD);
        }
    }

    if(num_files < num_map_workers) num_map_workers = num_files;

    for(int i = 1; i <= num_map_workers; i ++)
    {
        rc = MPI_Receive(&completed, 1, MPI_CHAR, i, i, MPI_COMM_WORLD, Stat);
    }
}

void map_worker()
{

}

void reduce_worker()
{

}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_size, rank;
    int dest, source, rc;
    MPI_Status Stat;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Get command-line params
    char *input_files_dir = argv[1];
    int num_files = atoi(argv[2]);
    int num_map_workers = atoi(argv[3]);
    int num_reduce_workers = atoi(argv[4]);
    char *output_file_name = argv[5];
    int map_reduce_task_num = atoi(argv[6]);

    // Identify the specific map function to use
    MapTaskOutput* (*map) (char*);
    switch(map_reduce_task_num){
        case 1:
            map = &map1;
            break;
        case 2:
            map = &map2;
            break;
        case 3:
            map = &map3;
            break;
    }

    // Distinguish between master, map workers and reduce workers
    if (rank == 0) {
        // TODO: Implement master process logic
        
        //
        
        
        //Step 3. (map) receive notificatation from workers after they complete
        //Step 4. (map) send more incompleted tasks to idle workers (if have)
        //Step 5. (reduce) send reduce task to the reduce worker
        //step 6: (gather) gather the final output and put into a single file

        printf("Rank (%d): This is the master process\n", rank);
    } else if ((rank >= 1) && (rank <= num_map_workers)) {
        // TODO: Implement map worker process logic
        //step 1. receive the files from master
        MPI_Recv()
        //step 2. run the map function and generate key-value pairs
        //step 3. partition (and store in separate file?)
        //step 4. send the partitioned filed to master?

        printf("Rank (%d): This is a map worker process\n", rank);
    } else {
        // TODO: Implement reduce worker process logic
        //step 1. receive the partion files from master
        //step 2. aggregate the values for each key into an array
        //step 3. send back to the master
        printf("Rank (%d): This is a reduce worker process\n", rank);
    }

    //Clean up
    MPI_Finalize();
    return 0;
}
