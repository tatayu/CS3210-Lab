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

int MAX = 50000000; //~50MB
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

int main(int argc, char **argv)
{
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

    if (num_files < num_map_workers)
    {
        num_map_workers = num_files;
    }

    //!create a type for stuct keys
    //type for store pair
    int nitems_storePair = 2;
    int blocklengths1[2] = {8, 1};

    MPI_Aint offsets1[2];
    offsets1[0] = (MPI_Aint)offsetof(storePair, key);
    offsets1[1] = (MPI_Aint)offsetof(storePair, val);
    MPI_Datatype types1[2] = {MPI_CHAR, MPI_INT};
    MPI_Datatype mpi_pairs_type;
    MPI_Type_create_struct(nitems_storePair, blocklengths1, offsets1, types1, &mpi_pairs_type);
    MPI_Type_commit(&mpi_pairs_type);

    // Identify the specific map function to use
    MapTaskOutput *(*map)(char *);
    switch (map_reduce_task_num)
    {
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

    //!MALLOC
    char *file_content = (char *)malloc(MAX);
    MapTaskOutput *output = (MapTaskOutput *)malloc(sizeof(MapTaskOutput));
    output->len = 0;
    output->kvs = NULL;

    storePair *reduce_pair = (storePair *)malloc(sizeof(storePair));
    reduce_pair->val = 0;

    storePartition *partition_table = (storePartition *)malloc(sizeof(storePartition));
    partition_table->pair = (storePair *)malloc(1000 * sizeof(storePair));
    partition_table->len = 0; //number of pairs

    storePair *master_pair = (storePair *)malloc(sizeof(storePair));
    master_pair->val = 0;

    // Distinguish between master, map workers and reduce workers
    //read from file
    if (rank == 0)
    {

        for (int i = 0; i < num_files; i++)
        {
            memset(file_content, 0, MAX);
            //Step 1. Read the files into buffer
            char *filepath = (char *)malloc(100 * sizeof(char));
            memcpy(filepath, input_files_dir, strlen(input_files_dir) + 1);
            char *slash = "/";
            char *txt = ".txt";
            strcat(filepath, slash);

            char num[10];
            sprintf(num, "%d", i);
            strcat(filepath, num);
            char *filename = strcat(filepath, txt);
            FILE *fp = fopen(filename, "r");

            if (fp == NULL)
            {
                printf("Error: could not open file %s", filename);
            }

            fseek(fp, 0, SEEK_END);
            size_t size = ftell(fp);

            rewind(fp);
            fread((void *)file_content, 1, size, fp);

            //Step 2. (map) send the data to the map worker
            if (i < num_map_workers)
            {
                MPI_Send(file_content, strlen(file_content), MPI_CHAR, i + 1, 0, MPI_COMM_WORLD);
            }
            else
            {
                //wait for idle worker then send the rest of the files
                char message;
                MPI_Recv(&message, 1, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Stat);
                if (message == '#')
                {
                    //send new file to that worker
                    MPI_Send(file_content, strlen(file_content), MPI_CHAR, Stat.MPI_SOURCE, 0, MPI_COMM_WORLD);
                }
            }
        }

        char terminating_message = '$';
        int working_map_worker = num_map_workers;
        while (working_map_worker > 0)
        {
            char message;
            MPI_Recv(&message, 1, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Stat);
            if (message == '#')
            {
                MPI_Send(&terminating_message, 1, MPI_CHAR, Stat.MPI_SOURCE, 0, MPI_COMM_WORLD);
                working_map_worker -= 1;
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        FILE *result = fopen("result.out", "w");
        for (int i = 0; i < num_reduce_workers; i++)
        {
            int len = 0;
            MPI_Recv(&len, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &Stat);

            for (int j = 0; j < len; j++)
            {
                MPI_Recv(master_pair, 1, mpi_pairs_type, Stat.MPI_SOURCE, 0, MPI_COMM_WORLD, &Stat);

                if (result != NULL)
                {
                    fprintf(result, "%s %d \n", master_pair->key, master_pair->val);
                }
            }
        }
        fclose(result);
    }
    else if ((rank >= 1) && (rank <= num_map_workers))
    {
        reduce_pair->val = -1;
        while (1)
        {
            memset(file_content, 0, MAX);
            MPI_Recv(file_content, MAX, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &Stat);
            if (file_content[0] == '$')
            {
                break;
            }
            output = map(file_content);
            for (int i = 0; i < output->len; i++)
            {
                int p = partition(output->kvs[i].key, num_reduce_workers);
                MPI_Send(&output->kvs[i], 1, mpi_pairs_type, num_map_workers + p + 1, num_map_workers + p + 1, MPI_COMM_WORLD);
            }

            for (int i = 1; i <= num_reduce_workers; i++)
            {
                MPI_Send(reduce_pair, 1, mpi_pairs_type, num_map_workers + i, num_map_workers + i, MPI_COMM_WORLD);
            }

            char message = '#';
            MPI_Send(&message, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    else if ((rank > num_map_workers) && (rank <= num_map_workers + num_reduce_workers))
    {
        int number_of_files = num_files;
        while (number_of_files > 0)
        {
            MPI_Recv(reduce_pair, 1, mpi_pairs_type, MPI_ANY_SOURCE, rank, MPI_COMM_WORLD, &Stat);

            if (reduce_pair->val == -1)
            {
                number_of_files -= 1;
            }
            else
            {
                bool is_duplicate = false;
                for (int k = 0; k < partition_table->len; k++)
                {
                    if (strcmp(reduce_pair->key, partition_table->pair[k].key) == 0)
                    {
                        partition_table->pair[k].val += reduce_pair->val;
                        is_duplicate = true;
                        break;
                    }
                }

                if (is_duplicate == false)
                {
                    memcpy(partition_table->pair[partition_table->len].key, reduce_pair->key, sizeof(partition_table->pair[partition_table->len].key));
                    partition_table->pair[partition_table->len].val = reduce_pair->val;
                    partition_table->len += 1;
                }
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Send(&partition_table->len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        for (int i = 0; i < partition_table->len; i++)
        {
            MPI_Send(&partition_table->pair[i], 1, mpi_pairs_type, 0, 0, MPI_COMM_WORLD);
        }
    }
    else
    {
        MPI_Barrier(MPI_COMM_WORLD);
        //do nothing
    }

    //Clean up
    free_map_task_output(output);
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
