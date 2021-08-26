/*******************************************************************
 * semaph.c
 * Demonstrates using semaphores to synchromize Linux processes created with fork()
 * Compile: gcc -pthread -o semaph semaph.c
 * Run: ./semaph
 * Partially adapted from https://stackoverflow.com/questions/16400820/c-how-to-use-posix-semaphores-on-forked-processes
 *Yu Xiaoxue 
 *A0187744N
 *******************************************************************/

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUF_SIZE 10

int main(int argc, char** argv)
{
    /*      loop variables          */
    int i;
    /*      shared memory key       */
    key_t shmkey;
    /*      shared memory id        */
    int shmid;
    /*      synch semaphore         */ /*shared */
    sem_t* full;
    sem_t* empty;
    sem_t* critical;
    /*      fork pid                */
    pid_t pid;
    /*      fork count              */
    unsigned int n;
    int *consumer_sum;
    int *produce;
    int *consume;
    int *producer_buffer;

    /* initialize a shared variable in shared memory */
    shmkey = ftok("/dev/null", 5); /* valid directory name and a number */
    printf("shmkey for consume_sum = %d\n", shmkey);
    shmid = shmget(shmkey, sizeof(int), 0644 | IPC_CREAT);
    if (shmid < 0) { /* shared memory error check */
        perror("shmget\n");
        exit(1);
    }
    consumer_sum = (int*)shmat(shmid, NULL, 0);
    *consumer_sum = 0;
    
    shmkey = ftok("/dev/null", 6);
    printf("shmkey for produce = %d\n", shmkey);
    shmid = shmget(shmkey, sizeof(int), 0644 | IPC_CREAT);
    if (shmid < 0) {
	perror("shmget\n");
	exit(1);
    }
    produce = (int*)shmat(shmid, NULL, 0);
    *produce = 0;
    
    shmkey = ftok("/dev/null", 7);
    printf("shmkey for consume = %d\n", shmkey);
    shmid = shmget(shmkey, sizeof(int), 0644 | IPC_CREAT);
    if(shmid < 0) {
        perror("shmget\n");
	exit(1);
    }
    consume = (int*)shmat(shmid, NULL, 0);
    *consume = 0;

    shmkey = ftok("/dev/null", 100);
    printf("shmkey for buffer = %d\n", shmkey);
    shmid = shmget(shmkey, sizeof(int)*100, 0644 | IPC_CREAT);
    if(shmid < 0) {
	perror("shmget\n");
	exit(1);
    }
    producer_buffer = shmat(shmid, NULL, 0);
    /********************************************************/

    /* initialize semaphores for shared processes */
    full = sem_open("fullSem", O_CREAT | O_EXCL, 0644, 0);
    /* name of semaphore is "fullSem", semaphore is reached using this name */

    empty = sem_open("emptySem", O_CREAT | O_EXCL, 0644, 10);
    critical = sem_open("critSem", O_CREAT | O_EXCL, 0644, 1);

    printf("Semaphore initialized.\n\n");

    /* fork child processes */
    for (i = 0; i < n; i++) {
        pid = fork();
        if (pid < 0) {
            /* check for error      */
            sem_unlink("fullSem");
	    sem_unlink("emptySem");
	    sem_unlink("critSem");
            sem_close(full);
	    sem_close(empty);
	    sem_close(critical);
            /* unlink prevents the semaphore existing forever */
            /* if a crash occurs during the execution         */
            printf("Fork error.\n");
        } else if (pid == 0)
            break; /* child processes */
    }

    /******************************************************/
    /******************   Consumer PROCESS  ***************/
    /******************************************************/
    if (pid != 0) {
	while(1)
	{
	    sem_wait(full);
	    sem_wait(critical);
	    int temp = producer_buffer[*consume];
	    *consumer_sum += temp;
	    printf("consume position: %d\n", *consume);
	    printf("number added is: %d\n", temp);
	    printf("CONSUMER SUM IS: %d\n", *consumer_sum);
	    *consume = (*consume + 1) % BUF_SIZE;
	    sem_post(critical);
	    sem_post(empty);
	    sleep(1);
	}

        /* wait for all children to exit */
        while (pid = waitpid(-1, NULL, 0)) {
            if (errno == ECHILD)
                break;
        }

        printf("\nParent: All children have exited.\n");

        /* shared memory detach */
	shmdt(consumer_sum);
	shmdt(consume);
	shmdt(produce);
	shmdt(producer_buffer);
        shmctl(shmid, IPC_RMID, 0);

        /* cleanup semaphores */
        sem_unlink("fullSem");
	sem_unlink("emptySem");
	sem_unlink("critSem");
        sem_close(full);
	sem_close(empty);
	sem_close(critical);
        /* unlink prevents the semaphore existing forever */
        /* if a crash occurs during the execution         */
        exit(0);
    }

    /******************************************************/
    /******************   PRODUCER PROCESS   **************/
    /******************************************************/
    else {
        while(1)
	{
	    sem_wait(empty); /* P operation */
	    sem_wait(critical);
 	    producer_buffer[*produce] = (rand() % 10) + 1;
	    printf("produce position: %d\n", *produce);
	    printf("NUMBER PRODUCED: %d\n", producer_buffer[*produce]);
	    *produce = (*produce + 1) % BUF_SIZE;
	    sem_post(critical);
            sem_post(full); /* V operation */
	    sleep(1);
	}
        exit(0);
    }
}
