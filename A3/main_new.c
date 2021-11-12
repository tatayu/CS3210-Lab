#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "tasks.h"
#include "utils.h"
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

int MAX = 20000000; //~20MB
typedef struct keypair {
    char key[8];
    int val;
} keys;

// /*********************************************************************************************************/
typedef struct MapkOutput {
    int len;                // Length of `kvs` array
    KeyValue *kvs;          // Array of KeyValue items
} output;


char *get_file_content(char *input_files_dir, int i)
{
    printf("6\n");
    char *filepath = input_files_dir;
    char *slash = "/";
    char *txt = ".txt";
    strcat(filepath, slash);
    printf("7\n");	

    char num[10];
    sprintf(num, "%d", i);
    printf("8\n");
    strcat(filepath, num);
    char *filename = strcat(filepath, txt);
    printf("%s\n", filepath);
    FILE *fp = fopen(filename, "r");

    if(fp == NULL)
    {
        printf("Error: could not open file %s", filename);
        //return 1;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    char *file_content = (char*)malloc(size);

    rewind(fp);
    fread((void*)file_content, 1, size, fp);
    
    return file_content;
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

    //!create a type for stuct keys**********************************************************************/
    printf("1\n");
    //type for keys
    int nitems_keys = 2;
    int blocklengths0[2] = {8, 1}; 

    MPI_Aint offsets0[2];
    offsets0[0] = (MPI_Aint) offsetof(keys, key);
    offsets0[1] = (MPI_Aint) offsetof(keys, val);
    MPI_Datatype types0[2] = {MPI_CHAR, MPI_INT};
    MPI_Datatype mpi_keys_type;
    MPI_Type_create_struct(nitems_keys, blocklengths0, offsets0, types0, &mpi_keys_type);
    MPI_Type_commit(&mpi_keys_type);
    
    printf("2\n");
    //type for store pair
    int nitems_storePair = 2;
    int blocklengths1[2] = {8, 1000}; //array ???????????????????
    
    MPI_Aint offsets1[2];
    offsets1[0] = (MPI_Aint) offsetof(storePair, key);
    offsets1[1] = (MPI_Aint) offsetof(storePair, val);
    MPI_Datatype types1[2] = {MPI_CHAR, MPI_INT};
    MPI_Datatype mpi_pairs_type;
    MPI_Type_create_struct(nitems_storePair, blocklengths1, offsets1, types1, &mpi_pairs_type);
    MPI_Type_commit(&mpi_pairs_type);

    printf("3\n");
    //type for store partition
    int nitems_storePartition = 2;
    int blocklengths2[2] = {1, 1000};
    
    MPI_Aint offsets2[2];
    offsets2[0] = (MPI_Aint) offsetof(storePartition, len);
    offsets2[1] = (MPI_Aint) offsetof(storePartition, pair);
    MPI_Datatype types2[2] = {MPI_INT, mpi_pairs_type};
    MPI_Datatype mpi_partitions_type;
    MPI_Type_create_struct(nitems_storePartition, blocklengths2, offsets2, types2, &mpi_partitions_type);
    MPI_Type_commit(&mpi_partitions_type);
    /*********************************************************************************************************/
    
    printf("4\n");
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
    printf("5\n");

    // Distinguish between master, map workers and reduce workers
    if (rank == 0) {
        
        for(int i = 0; i < num_files; i ++)
        {
            //Step 1. Read the files into buffer
	        char *file_content = (char *)malloc(MAX * (sizeof(char)));
            file_content = get_file_content(input_files_dir, i);
	        printf("file sent: %s \n", file_content);
	        printf("get file\n");	

            //Step 2. (map) send the data to the map worker
            if(i < num_map_workers)
            {
                printf("[MASTER]sending files to map workers...\n");
                MPI_Send(file_content, strlen(file_content), MPI_CHAR, i + 1, 0, MPI_COMM_WORLD);
            }
            else
            {
                //wait for idle worker then send the rest of the files
                printf("[MASTER]receiving from any completed worker!\n");
                char *message;
                MPI_Recv(message, MAX, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);
                if(*message == 'O')
                {
                    //send new file to that worker
                    printf("[MASTER]sending files to idel map workers...\n");
                    MPI_Send(file_content, strlen(file_content), MPI_CHAR, Stat.MPI_SOURCE, 0, MPI_COMM_WORLD);
                }
            }
        }

        
        
        char terminating_message = '$';
        int working_map_worker = num_map_workers;
        while(working_map_worker > 0)
        {
            char message;
            printf("[MASTER]receiving terminating message from map workers...\n");
            MPI_Recv(&message, 1, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &Stat);
            printf("[MASTER]receive %c\n", message);
	    if(message == '#')
            {
                printf("[MASTER]Replying back terminating message to map workers...\n");
                MPI_Send(&terminating_message, 1, MPI_CHAR, Stat.MPI_SOURCE, 0, MPI_COMM_WORLD);
		working_map_worker -= 1;
            }
            
        }
        printf("[MASTER] frist barrier\n");
        MPI_Barrier(MPI_COMM_WORLD);
        
        printf("[MASTER] second barrier\n");
        MPI_Barrier(MPI_COMM_WORLD);

        //!barrier
        //TODO: Gather results from all reduce using MPI_Gather????
        // KeyValue *final_result = (KeyValue *)malloc(1000 * sizeof(KeyValue));
        // //MPI_Gather(?, ?, ?, final_result)
        
        // //write to output file
        // FILE *write;
        // write =fopen("result.output", "w");
        // fputs(final_result, write);
        // fclose(write);

        printf("Rank (%d): This is the master process\n", rank);
    } else if ((rank >= 1) && (rank <= num_map_workers)) {

        MapTaskOutput *output = (MapTaskOutput *)malloc(sizeof(MapTaskOutput));
        printf("map1\n");
	    storePartition *part= (storePartition *)malloc(num_reduce_workers * sizeof(storePartition));
        printf("map2\n");
	    for(int i = 0; i < num_reduce_workers; i ++)
        {
	        printf("map3\n");
            part[i].len = 0;
            //malloc pair here?????
	        printf("map4\n");
            part[i].pair = (storePair *)malloc(1000 * sizeof(storePair));
        }

        //TODO: need to change condition or receive a notification from master saying no more file to process?????
        while(1) 
        {
	        printf("map5\n");
            char *file_content = (char*)malloc(MAX);
            printf("[MAP]receiving files from master...\n");
            MPI_Recv(file_content, MAX, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &Stat);
            printf("[MAP]received!\n");
	         printf("[MAP]file content %s \n", &file_content[0] );
	        if(file_content[0] = '$')
            {
               //break;
            }
            
            output = map(file_content);

            for(int i = 0; i < output->len; i ++)
            {	
		        printf("[MAP]calculating partition...\n");
                int p = partition(output->kvs[i].key, num_reduce_workers);
		        printf("[MAP]partition value: %d", p);
                part[p].pair[part[p].len].val = (int *)malloc(1000 * sizeof(int));
                printf("map5\n");
		        memcpy(part[p].pair[part[p].len].key, output->kvs[i].key, sizeof(part[p].pair[part[p].len].key));
                printf("map6\n");
		        part[p].pair[part[p].len].val[0] = output->kvs[i].val;
                printf("map7\n");
		        part[p].len += 1;
		        printf("map8\n");
            } 
            
            char message = '#';
            printf("[MAP]sending terminating message to master...\n");
            MPI_Send(&message, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD); 
            
      	    break;	    
        }


        //!barrier 
        printf("[MAP]first barrier!\n");
        MPI_Barrier(MPI_COMM_WORLD);
        
        //after collecting the partition from ALL the files processed by this worker, sent each partition to corresponding reduce worker
        for(int i = 1; i <= num_reduce_workers; i++)
        {
            printf("[MAP]sending partitions to reduce workers...\n");
            MPI_Send(&part[i], sizeof(part[i]), mpi_partitions_type, num_map_workers + i, num_map_workers + i, MPI_COMM_WORLD);
        }

        //!barrier
        printf("[MAP]second barrier!\n");
        MPI_Barrier(MPI_COMM_WORLD);
        
        printf("Rank (%d): This is a map worker process\n", rank);

    } else {
	//while(1);	
	    printf("reduce 1\n");
        bool first_part = false;
        printf("reduce 2\n");	
        storePartition *primary_part = (storePartition *)malloc(sizeof(storePartition));
	    printf("reduce 3\n");
        primary_part->pair = (storePair *)malloc(1000 * sizeof(storePair));
	//receive partitions generated by all the files
        
        //!barrier
        printf("[REDUCE]first barrier!\n");
        MPI_Barrier(MPI_COMM_WORLD);
        
        for(int k = 0; k < num_files; k ++)
        {   
	        printf("reduce 4\n");
            if(first_part == false)
            { 
                printf("[REDUCE]Receiving the first partition from map worker!\n");
                MPI_Recv(primary_part, sizeof(primary_part), mpi_partitions_type, MPI_ANY_SOURCE, rank, MPI_COMM_WORLD, &Stat);
                printf("[REDUCE]received partion from map worker!!!\n");
		        first_part = true;
            }
            else
            {
                storePartition *part = (storePartition *)malloc(sizeof(storePartition));
                printf("[REDUCE]Receiving the partitions from map workers...\n");
                MPI_Recv(part, MAX, mpi_partitions_type, rank, 0, MPI_COMM_WORLD, &Stat);
                
                //check the keys and compare and add on to the primary part
                printf("[REDUCE]check the keys and compare and add on to the primary part...\n");
                for(int i = 0; i < part->len; i ++)
                {
                    bool is_same = false;
                    for(int j = 0; j < primary_part->len; j ++ )
                    {
                        //can aggregate
                        if(part->pair[i].key == primary_part->pair[j].key)
                        {
                            size_t index = sizeof(primary_part->pair[j].val) / sizeof(int);
                            primary_part->pair[j].val[index] = part->pair[i].val[0];
                            is_same = true;
                            break;
                        }
                    }

                    //cannot aggregate, add new key to pair array
                    if(is_same == false)
                    {
                        primary_part->len += 1;
                        memcpy(primary_part->pair[primary_part->len].key, part->pair[i].key, sizeof(primary_part->pair[primary_part->len].key));  
                        primary_part->pair[primary_part->len].val[0] = part->pair[i].val[0];
                    }    
                }
            }
        }
        
        //do reduce work and combine the result
        KeyValue *result = (KeyValue *)malloc(primary_part->len * sizeof(KeyValue));
        for(int i = 0; i < primary_part->len; i ++)
        {
            int length = sizeof(primary_part->pair[i].val) / sizeof(int);
            result[i] = reduce(primary_part->pair[i].key, primary_part->pair[i].val, length);
        }

        printf("[REDUCE]second barrier!\n");
        //!barrier
        MPI_Barrier(MPI_COMM_WORLD);
        printf("[REDUCE]Sending result back to master!\n");
        MPI_Send(result, sizeof(result), mpi_keys_type, 0, 0, MPI_COMM_WORLD);
    
        printf("Rank (%d): This is a reduce worker process\n", rank);
    }

    //Clean up
    MPI_Finalize();
    return 0;
}
