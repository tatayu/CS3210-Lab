/**
 * CS3210 - Blocking communication in MPI.
 */

#include <mpi.h>
#include <stdio.h>

int main(int argc,char *argv[])
{
	int numtasks, rank, dest, source, rc, count, tag=1;  
	char inmsg, outmsg='x';
	float array[10] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
	float rarray[10];
	MPI_Status Stat;

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0)	{
		dest = 1;
		source = 1;
		rc = MPI_Send(&array, 10, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
		rc = MPI_Recv(&rarray, 10, MPI_FLOAT, source, tag, MPI_COMM_WORLD, &Stat);
		//rc = MPI_Send(&outmsg, 1, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
		
	} 	else if (rank == 1)	{
		dest = 0;
		source = 0;
		rc = MPI_Recv(&rarray, 10, MPI_FLOAT, source, tag, MPI_COMM_WORLD, &Stat);
		rc = MPI_Send(&array, 10, MPI_FLOAT, dest, tag, MPI_COMM_WORLD);
	}

	rc = MPI_Get_count(&Stat, MPI_CHAR, &count);
	printf("Task %d: Received %d char(s) from task %d with tag %d \n",
       		rank, count, Stat.MPI_SOURCE, Stat.MPI_TAG);

	MPI_Finalize();
	
	return 0;
}

