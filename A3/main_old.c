#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "tasks.h"
#include "utils.h"
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

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

int MAX = 50000000; //~20MB
typedef struct _storePair
{
    char key[8];
    int val;
} storePair;

typedef struct _storePartition
{
    int len; //lenght of the pair
    storePair *pair;
} storePartition;

int main(int argc, char** argv) {
    //!CLOCK
    long long before, after;
    before = wall_clock_time();
    
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

    if(num_files < num_map_workers)
    {
	    num_map_workers = num_files;
    }

    //!create a type for stuct keys**********************************************************************/
    //type for store pair
    int nitems_storePair = 2;
    int blocklengths1[2] = {8, 1}; 
    
    MPI_Aint offsets1[2];
    offsets1[0] = (MPI_Aint) offsetof(storePair, key);
    offsets1[1] = (MPI_Aint) offsetof(storePair, val);
    MPI_Datatype types1[2] = {MPI_CHAR, MPI_INT};
    MPI_Datatype mpi_pairs_type;
    MPI_Type_create_struct(nitems_storePair, blocklengths1, offsets1, types1, &mpi_pairs_type);
    MPI_Type_commit(&mpi_pairs_type);

    /*********************************************************************************************************/

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

    //!MALLOC*******************************************************************************************************
    char *file_content = (char*)malloc(MAX);
    MapTaskOutput *output = (MapTaskOutput *)malloc(sizeof(MapTaskOutput));
    output -> 0;
    output -> kvs=NULL;
    //MapTaskOutput *output;
    
    storePartition *part[num_reduce_workers];
    for(int i = 0; i < num_reduce_workers; i ++)
    {
        part[i] = (storePartition *)malloc(sizeof(storePartition));
        part[i]->len = 0;
        part[i]->pair = (storePair *)malloc(1000 * sizeof(storePair));
    }

    storePair *reduce_pair = (storePair *)malloc(sizeof(storePair));
    reduce_pair->val = 0;
    
    storePartition *partition_table = (storePartition *)malloc(sizeof(storePartition));
    partition_table->pair = (storePair *)malloc(1000 * sizeof(storePair));
    partition_table->len = 0; //number of pairs

    storePair *master_pair = (storePair *)malloc(sizeof(storePair));
    master_pair->val = 0;

    // Distinguish between master, map workers and reduce workers
    //read from file **************************************************************************************************
    if (rank == 0) {
       
        for(int i = 0; i < num_files; i ++)
        {
            memset(file_content, 0, MAX);
            //Step 1. Read the files into buffer*******************************************************************************************
             char *filepath = (char *)malloc(100*sizeof(char));
	        memcpy(filepath, input_files_dir, strlen(input_files_dir)+1);
            //printf("%s\n", filepath);
            char *slash = "/";
            char *txt = ".txt";
            strcat(filepath, slash);

            char num[10];
            sprintf(num, "%d", i);
            strcat(filepath, num);
            char *filename = strcat(filepath, txt);
            //printf("%s\n", filepath);
            FILE *fp = fopen(filename, "r");

            if(fp == NULL)
            {
                printf("Error: could not open file %s", filename);
                //return 1;
            }

            fseek(fp, 0, SEEK_END);
            size_t size = ftell(fp);

            rewind(fp);
            fread((void*)file_content, 1, size, fp);
	        //printf("file sent: %s \n", file_content);
	        //printf("get file\n");	

            //Step 2. (map) send the data to the map worker******************************************************************
            if(i < num_map_workers)
            {
                //printf("[MASTER]sending files to map workers...\n");
                MPI_Send(file_content, strlen(file_content), MPI_CHAR, i + 1, 0, MPI_COMM_WORLD);
            }
            else
            {
                //wait for idle worker then send the rest of the files
                //printf("[MASTER]receiving from any completed worker!\n");
                char message;
                MPI_Recv(&message, 1, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Stat);
                if(message == '#')
                {
                    //send new file to that worker
                    //printf("[MASTER]sending files to idel map workers...\n");
                    MPI_Send(file_content, strlen(file_content), MPI_CHAR, Stat.MPI_SOURCE, 0, MPI_COMM_WORLD);
                }
            }
        }
        
        char terminating_message = '$';
        int working_map_worker = num_map_workers;
        //printf("num_map_workers: %d\n", num_map_workers);
	    while(working_map_worker > 0)
        {
	        //printf("working_map_workers: %d\n", working_map_worker);
            char message;
            //printf("[MASTER]receiving terminating message from map workers...\n");
            MPI_Recv(&message, 1, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Stat);
            //printf("[MASTER]receive %c\n", message);
	        if(message == '#')
            {
                //printf("[MASTER]Replying back terminating message to map workers...\n");
                MPI_Send(&terminating_message, 1, MPI_CHAR, Stat.MPI_SOURCE, 0, MPI_COMM_WORLD);
		        working_map_worker -= 1;
            }
            
        }

        //printf("[MASTER] frist barrier\n");
        MPI_Barrier(MPI_COMM_WORLD);
        FILE *result = fopen(output_file_name, "w");
        for(int i = 0; i < num_reduce_workers; i++)
        {
            int len = 0;
            //printf("[MASTER]Receiving length of pairs from reduce workers...\n");
            MPI_Recv(&len, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Stat);
            //printf("[MASTER]Length of pairs received!!!\n");
            
            for(int j = 0; j < len; j ++)
            {
                //printf("[MASTER]Receiving pair %d from reduce workers...\n", j);
                MPI_Recv(master_pair, 1, mpi_pairs_type, Stat.MPI_SOURCE, 0, MPI_COMM_WORLD, &Stat);
                //printf("[MASTER]Received pair %d from reduce workers!!!\n", j);
                //printf("[MASTER]key: %s\n", master_pair->key);
                //printf("[MASTER]val: %d\n", master_pair->val);

                if(result != NULL)
                {
                    fprintf(result, "%s %d \n", master_pair->key, master_pair->val);
                }
            }
        }
        fclose(result);
    
        printf("Rank (%d): This is the master process\n", rank);
        
    } else if ((rank >= 1) && (rank <= num_map_workers)) {
        
        while(1) 
        {   
            memset(file_content, 0, MAX);
            //printf("[MAP]receiving files from master...\n");
            MPI_Recv(file_content, MAX, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &Stat);
            //printf("[MAP]received!\n");
	        //printf("[MAP]file content %s \n", &file_content[0] );
	        if(file_content[0] == '$')
            {
               break;
            }
            
            output = map(file_content);
            for(int i = 0; i < output->len; i ++)
            {	
                int p = partition(output->kvs[i].key, num_reduce_workers);        
		        //printf("[MAP]partition value: %d\n", p);
		        memcpy(part[p]->pair[part[p]->len].key, output->kvs[i].key, sizeof(part[p]->pair[part[p]->len].key));
		        part[p]->pair[part[p]->len].val = output->kvs[i].val;
                //printf("[MAP]part p pair key-value: %s %d\n", part[p]->pair[part[p]->len].key, part[p]->pair[part[p]->len].val);
		        part[p]->len += 1;
            } 
            
            char message = '#';
            //printf("[MAP]sending terminating message to master...\n");
            MPI_Send(&message, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);     
        }
        
        //after collecting the partition from ALL the files processed by this worker, sent each partition to corresponding reduce worker
        for(int i = 1; i <= num_reduce_workers; i++)
        {
            //printf("[MAP]sending length...\n");
            MPI_Send(&part[i - 1]->len, 1, MPI_INT, num_map_workers + i, num_map_workers + i, MPI_COMM_WORLD);
            //printf("[MAP]length sent!!!\n");
            
            for(int j = 0; j < part[i - 1]->len; j ++)
            {
                //printf("[MAP]sending pair...\n");
                MPI_Send(&part[i - 1]->pair[j], 1, mpi_pairs_type, num_map_workers + i, num_map_workers + i, MPI_COMM_WORLD);
                //printf("[MAP]pair sent!!!\n");
            }
	    }

        //printf("[MAP]first barrier!\n");
        MPI_Barrier(MPI_COMM_WORLD);     
        printf("Rank (%d): This is a map worker process\n", rank);

    } 
    else if((rank > num_map_workers) && (rank <= num_map_workers + num_reduce_workers))
    {
        for(int i = 0; i < num_map_workers; i ++)
        {   
            int len; //number of pairs
            //printf("[REDUCE]Receiving length...\n");
            MPI_Recv(&len, 1, MPI_INT, MPI_ANY_SOURCE, rank, MPI_COMM_WORLD, &Stat);   
            //printf("[REDUCE]Length received!!!\n");
            
            
            for(int j = 0; j < len; j ++)
            {
                //printf("[REDUCE]Receiving the pair %d from map worker...\n", j);
                MPI_Recv(reduce_pair, 1, mpi_pairs_type, Stat.MPI_SOURCE, rank, MPI_COMM_WORLD, &Stat);
                //printf("[REDUCE]received pair %d from map worker!!!\n", j);
                //printf("[REDUCE]key: %s\n", reduce_pair->key);
                //printf("[REDUCE]val: %d\n", reduce_pair->val);
                
                bool is_duplicate = false;
                for(int k = 0; k < partition_table->len; k ++)
                {
                    if(strcmp(reduce_pair->key, partition_table->pair[k].key) == 0)
                    {
                        //printf("old: %d", partition_table->pair[k].val);
                        partition_table->pair[k].val += reduce_pair->val;
                        //printf("key: %s, add: %d, new: %d\n", reduce_pair->key, reduce_pair->val,  partition_table->pair[k].val);
                        is_duplicate = true;
                        break;
                    }
                }

                if(is_duplicate == false)
                {
                    //printf("[REDUCE]adding new keys to the partition table...\n");
                    memcpy(partition_table->pair[partition_table->len].key, reduce_pair->key, sizeof(partition_table->pair[partition_table->len].key));
                    partition_table->pair[partition_table->len].val = reduce_pair->val;
                    partition_table->len += 1;         
                }
            }
        }

        //printf("[REDUCE]first barrier!\n");
        MPI_Barrier(MPI_COMM_WORLD); 
        //printf("[REDUCE]sending length of pairs back to master...\n");
        MPI_Send(&partition_table->len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        //printf("[REDUCE]length of pairs sent!!!\n");
        
        //printf("[REDUCE] partition_table length: %d\n", partition_table->len);
        for(int i = 0; i < partition_table->len; i ++)
        {
            //printf("[REDUCE]sending pairs back to master...\n");
            MPI_Send(&partition_table->pair[i], 1, mpi_pairs_type, 0, 0, MPI_COMM_WORLD);
            //printf("[REDUCE]pairs sent!!!\n");
        }
        printf("Rank (%d): This is a reduce worker process\n", rank);
    }
    else
    {
        MPI_Barrier(MPI_COMM_WORLD);
        printf("Rank (%d): This is a idle worker process\n", rank);
    }


    //Clean up
    free_map_task_output(output);
    for(int i = 0; i < num_reduce_workers; i ++)
    {
	    free(part[i]);	
    }
    free(file_content);
    free(reduce_pair);
    free(partition_table);
    free(master_pair);
    MPI_Finalize();
    //!clock end
    after = wall_clock_time();
    fprintf(stderr, "Operation took %1.2f seconds\n", ((float)(after - before)) / 1000000000);
    return 0;
}
