/*******************************************************************
 * prod-con-threads.c
 * Producer-consumer synchronisation problem in C
 * Compile: gcc -pthread -o prodcont prod-con-threads.c
 * Run: ./prodcont
 * Yu Xiaoxue
 * A0187744N
 *******************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define PRODUCERS 2
#define CONSUMERS 1
#define BUF_SIZE 10

pthread_mutex_t lock;
pthread_mutex_t prod_lock;
pthread_cond_t produce_cv;
pthread_cond_t consume_cv;

int producer_buffer[BUF_SIZE] = {0};
int consumer_sum = 0;
int produce = 0, consume = 0;
int c = 0;

void* producer(void* threadid)
{	
    while(1)
    {	
	pthread_mutex_lock(&prod_lock);
   	pthread_mutex_lock(&lock);
	
	producer_buffer[produce] = (rand() % 10) + 1;	
	while((produce + 1) % BUF_SIZE == consume)
   	{
	    printf("Buffer is full! Waiting... \n");
	    pthread_cond_wait(&consume_cv, &lock);
    	}
      	
    	printf("produce position: %d \n", produce);
	printf("NUMBER PRODUCED: %d \n", producer_buffer[produce]);
	produce = (produce + 1) % BUF_SIZE;
    	
	pthread_mutex_unlock(&lock);
	pthread_mutex_unlock(&prod_lock);
	
	pthread_cond_signal(&produce_cv);
	
	sleep(1);
    }
}

void* consumer(void* threadid)
{
    while(1)
    {
	pthread_mutex_lock(&lock);

	while(produce == consume)
        {
	    printf("Buffer is empty! Waiting... \n");
	    pthread_cond_wait(&produce_cv, &lock);
        }

	printf("consume position: %d \n", consume);
    	consumer_sum += producer_buffer[consume];
	printf("number added is: %d \n", producer_buffer[consume]);
	consume = (consume + 1) % BUF_SIZE;
    	printf("CONSUMER SUM IS: %d \n", consumer_sum);
    	
	pthread_mutex_unlock(&lock);

    	pthread_cond_signal(&consume_cv);

	sleep(1);
    }
}

int main(int argc, char* argv[])
{
    clock_t start = clock(), diff;
    pthread_t producer_threads[PRODUCERS];
    pthread_t consumer_threads[CONSUMERS];
    int producer_threadid[PRODUCERS];
    int consumer_threadid[CONSUMERS];

    int rc;
    int t1, t2;
    for (t1 = 0; t1 < PRODUCERS; t1++) {
        int tid = t1;
        producer_threadid[tid] = tid;
        printf("Main: creating producer %d\n", tid);
        rc = pthread_create(&producer_threads[tid], NULL, producer,
                (void*) &producer_threadid[tid]);
        if (rc) {
            printf("Error: Return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for (t2 = 0; t2 < CONSUMERS; t2++) {
        int tid = t2;
        consumer_threadid[tid] = tid;
        printf("Main: creating consumer %d\n", tid);
        rc = pthread_create(&consumer_threads[tid], NULL, consumer,
                (void*) &consumer_threadid[tid]);
        if (rc) {
            printf("Error: Return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for(t1 = 0; t1 < PRODUCERS; t1 ++)
    {
	pthread_join(producer_threads[t1], NULL);
    }

    for(t2 = 0; t2 < CONSUMERS; t2 ++) 
    {
	pthread_join(consumer_threads[t2], NULL);
    }

    pthread_exit(NULL);
}
