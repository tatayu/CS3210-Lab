/*******************************************************************
 * race-condition.c
 * Demonstrates a race condition.
 * Compile: gcc -pthread -o race race-condition.c
 * Run: ./race
 *******************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ADD_THREADS 4
#define SUB_THREADS 4

int global_counter;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int count = 0;
pthread_cond_t count_threshold_cv;

void* add(void* threadid)
{
    long tid = *(long*) threadid;
    
    pthread_mutex_lock(&lock);
    count ++;
    global_counter++;
    printf("add thread #%ld increamented global_counter!\n", tid);
    //printf("count: %d\n",count);
    
    if (count == ADD_THREADS){
	    printf("count: %d\n", count);
	    pthread_cond_broadcast(&count_threshold_cv);
	    printf("Just sent signal. \n");
    }
    
    pthread_mutex_unlock(&lock);
    
    sleep(rand() % 2);
   
}

void* sub(void* threadid)
{
    printf("Starting watch...\n");
    long tid = *(long*) threadid;
    
    pthread_mutex_lock(&lock);

    printf("count: %d\n", count);
    while(count < ADD_THREADS){
	    printf("waiting...\n");
	    pthread_cond_wait(&count_threshold_cv, &lock);
	    printf("Condition signal received!\n");
    }
    
    global_counter--;
    printf("sub thread #%ld decremented global_counter! \n", tid);
    
    pthread_mutex_unlock(&lock);
    
    sleep(rand() % 2);
    
}

int main(int argc, char* argv[])
{
    global_counter = 10;
    pthread_t add_threads[ADD_THREADS];
    pthread_t sub_threads[SUB_THREADS];
    long add_threadid[ADD_THREADS];
    long sub_threadid[SUB_THREADS];
    
    pthread_cond_init (&count_threshold_cv, NULL);

    int rc;
    long t1, t2;
    
    for(t2 = 0; t2 < SUB_THREADS; t2++) {
	int tid = t2;
	sub_threadid[tid] = tid;
	printf("main thread: creating sub thread %d\n", tid);
	rc = pthread_create(&sub_threads[tid], NULL, sub, (void*) &sub_threadid[tid]);
	if(rc) {
	    printf("Return code from pthread_create() is %d\n", rc);
	    exit(-1);
	}
    }

    for (t1 = 0; t1 < ADD_THREADS; t1++) {
        int tid = t1;
        add_threadid[tid] = tid;
        printf("main thread: creating add thread %d\n", tid);
        rc = pthread_create(&add_threads[tid], NULL, add,
                (void*) &add_threadid[tid]);
        if (rc) {
            printf("Return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
/*
    for (t2 = 0; t2 < SUB_THREADS; t2++) {
        int tid = t2;
        sub_threadid[tid] = tid;
        printf("main thread: creating sub thread %d\n", tid);
        rc = pthread_create(&sub_threads[tid], NULL, sub,
                (void*) &sub_threadid[tid]);
        if (rc) {
            printf("Return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }*/

    for(t1 = 0; t1 < ADD_THREADS; t1++){
	    pthread_join(add_threads[t1], NULL);
    }

    for(t2 = 0; t2 < SUB_THREADS; t2++) {
	    pthread_join(sub_threads[t2], NULL);
    }

    printf("### global_counter final value = %d ###\n",
            global_counter);
    pthread_exit(NULL);
}
