#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "tasks.h"
#include "utils.h"
#include <string.h>
#include <stddef.h>

int MAX = 20000000; //~20MB
typedef struct keypair {
    char key[8];
    int val;
} keys;

typedef struct MapkOutput {
    int len;                // Length of `kvs` array
    KeyValue *kvs;          // Array of KeyValue items
} output;

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


void master(char *input_files_dir, int num_files, int num_map_workers)
{
    //int reduce_worker_id = num_map_workers + 1;
    for(int i = 0; i < num_files; i ++)
    {
        //Step 1. Read the files into buffer
        char *file_content = get_file_content(input_files_dir, i);

        //Step 2. (map) send the data to the map worker
        if(i < num_map_workers)
        {
            rc = MPI_Send(file_content, strlen(file_content), MPI_CHAR, i + 1, 0, MPI_COMM_WORLD);
        }
        else
        {
            //wait for idle worker then send the rest of the files
            printf("receiving from any completed worker!")
            char *message;
            MPI_Recv(message, MAX, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, Stat);
            if(message == 'O')
            {
                //send new file to that worker
                MPI_Send(file_content, strlen(file_content), MPI_CHAR, Stat.MPI_SOURCE, 0, MPI_COMM_WORLD);
            }
        }
    }

    
    //TODO: Gather results from all reduce using MPI_Gather????
    KeyValue *final_result = (KeyValue *)malloc(1000 * sizeof(KeyValue));
    MPI_Gather(?, ?, ?, final_result)
    
    //write to output file
    FILE *write;
    write =fopen("result.output", "w");
    fputs(final_result, write);
    fclose(write);
}


typedef struct _storePair
{
    char key[8];
    int *val;
} storePair;

typedef struct _storePartition
{
    int len; //lenght of the pair
    storePair *pair;
} storePartition;

/*create a type for stuct key**********************************************************************/
int nitems_keys = 2;
int blocklengths[2] = {8, 1000}; //array ???????????????????
MPI_Datatype types[2] = {MPI_CHAR, MPI_INT};
MPI_Datatype mpi_keys_type;
MPI_Aint offsets[2];

offsets[0] = offsetof(keys, key);
offsets[1] = offsetof(keys, val); //offset of an array????????????????????

MPI_Type_create_struct(nitems_keys, blocklengths, offsets, types, &mpi_key_type);
MPI_Type_commit(&mpi_keys_type);

/*create a type for strcut output******************************************************************/
int nitems_output = 2;
int blocklengths1[2] = {1, 1000};
MPI_Datatype types1[2] = {MPI_INT, mpi_keys_type};
MPI_Datatype mpi_partition_type;
MPI_Aint offsets1[2];

offsets1[0] = offsetof(output, len);
offsets1[1] = offsetof(output, pairs);

MPI_Type_create_struct(nitems_output, blocklengths1, offsets1, types1, &mpi_partition_type);
MPI_Type_commit(&mpi_partition_type);

/***************************************************************************************************/

void map_worker(MapTaskOutput* (*map) (char*) ,int rank, MPI_Status Stat, int num_reduce_workers)
{
    
    storePartition *part= (storePartition *)malloc(num_reduce_workers * sizeof(storePartition));
    for(int i = 0; i < num_reduce_workers; i ++)
    {
        part[i].len = 0;
        //malloc pair here?????
        part[i].pair = (storePair *)malloc(1000 * sizeof(storePair));
    }

    //TODO: need to change condition or receive a notification from master saying no more file to process?????
    while(1) 
    {
        char *file_content = (char*)malloc(MAX);
        MPI_Recv(file_content, MAX, MPI_CHAR, 0, 0, MPI_COMM_WORLD, Stat);
        
        MapTaskOutput *output = (MapTaskOutput *)malloc(sizeof(MapTaskOutput));
        output = map(file_content);

        for(int i = 0; i < output->len, i ++)
        {
            int p = partition(output->kvs[i]->key, num_reduce_workers);

            part[p]->pair[part[p].len]->val = (int *)malloc(1000 * sizeof(int));
            part[p]->pair[part[p].len]->key = output->kvs[i]->key;
            part[p]->pair[part[p].len]->val[0] = output->kvs[i]->val;
            part[p].len += 1;
        }        
    }

    //after collecting the partition from all the files processed by this worker, sent each partition to corresponding reduce worker
    for(int i = 0; i < num_reduce_workers; i++)
    {
        MPI_Send(output, sizeof(part[i]), mpi_partition_type, rank, 0, MPI_COMM_WORLD);
    }
}


void reduce_worker(int rank, int num_reduce_workers, int num_files)
{
    //primary_part is the first received partition and the rest of the partition will be adding to primary_part
    bool first_part = false;  
    storePartition primary_part = (storePartition *)malloc(sizeof(storePartition));

    //receive partitions generated by all the files
    for(int k = 0; k < num_files; k ++)
    {
        if(first_part == false)
        {
            MPI_Recv(primary_part, MAX, mpi_output_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, Stat);
            first_part = true;
        }
        else
        {
            storePartition part = (storePartition *)malloc(sizeof(storePartition));
            MPI_Recv(part, MAX, mpi_output_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, Stat);
            
            //check the keys and compare and add on to the primary part
            for(int i = 0; i < part->len; i ++)
            {
                bool is_same = false;
                for(int j = 0; j < primary_part->len; j ++ )
                {
                    //can aggregate
                    if(part->pair[i]->key == primary_part->pair[j]->key)
                    {
                        size_t index = sizeof(primary_part->pair[j]->val) / sizeof(int);
                        primary_part->pair[j]->val[index] = part->pair[i].val[0];
                        is_same = true;
                        break;
                    }
                }

                //cannot aggregate, add new key to pair array
                if(is_same == false)
                {
                    primary_part->len += 1;
                    primary_part->pair[len]->key = part.pair[i];  
                    primary_part->pair[len]->val[0]
                }    
            }
        }
    }
    
    //do reduce work and combine the result
    KeyValue *result = (KeyValue *)malloc(primary_part.len * sizeof(KeyValue));
    for(int i = 0; i < primary_part.len; i ++)
    {
        int length = sizeof(primary_part->pair[i]->val) / sizeof(int);
        result[i] = reduce(primary_part->pair[i]->key, primary_part->pair[i]->val, length);
    }

    //TODO: send back to master (need to define a new MIP type????)
    MPI_Send(result, sizeof(result), ?, 0, 0, MPI_COMM_WORLD);
    
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

    //TODO: store in buffer all declared in main and pass to the functions?
    
    
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
        master(input_files_dir, num_files, num_map_workers);
        //step 1. read file into buffer
        //step 2. distribute the files to all map workers
        //Step 3. (map) receive notificatation from workers after they complete
        //Step 4. (map) send more incompleted tasks to idle workers (if have)


        printf("Rank (%d): This is the master process\n", rank);
    } else if ((rank >= 1) && (rank <= num_map_workers)) {
        // TODO: Implement map worker process logic
        map_worker(map , rank, Stat, num_reduce_workers);


        //step 1. receive the files from master
        //step 2. run the map function and generate key-value pairs
        //step 3. generate partitions for each reduce worker
        //step 4. send the partitions to corresponding reduce worker

        printf("Rank (%d): This is a map worker process\n", rank);
    } else {
        // TODO: Implement reduce worker process logic
        reduce_worker(rank, num_reduce_workers, num_files);

        //step 1. receive the partion files from master
        //step 2. aggregate the values for each key into an array
        //step 3. send back to the master
        printf("Rank (%d): This is a reduce worker process\n", rank);
    }

    //Clean up
    MPI_Finalize();
    return 0;
}
