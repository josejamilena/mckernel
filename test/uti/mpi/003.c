#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <mpi.h>
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <sched.h>
#include "util.h"

//#define DEBUG
#ifdef DEBUG
#define dprintf printf
#else
#define dprintf {}
#endif

#define SZENTRY_DEFAULT (65536) /* Size of one slot */
#define NENTRY_DEFAULT 10000 /* Number of slots */

void sendrecv(int rank, int nentry, char **sendv, char **recvv, int szentry,
	      int src, int dest, MPI_Request *reqs, MPI_Status *status,
	      double usec)
{
	int i;

	if (rank == 1) {
		for (i = 0; i < nentry; i++) {
			if (i % (nentry / 10) == 0) {
				printf("s"); fflush(stdout);
			}
			MPI_Isend(sendv[0], szentry, MPI_CHAR, dest, 0,
				  MPI_COMM_WORLD, &reqs[i]);
		}
		printf("\n"); fflush(stdout);
		MPI_Waitall(nentry, reqs, status);
	} else {
		for (i = 0; i < nentry; i++) {
			if (i % (nentry / 10) == 0) {
				printf("r"); fflush(stdout);
			}
			MPI_Irecv(recvv[0], szentry, MPI_CHAR, src, 0,
				  MPI_COMM_WORLD, &reqs[i]);
		}
		usleep(usec);
		MPI_Waitall(nentry, reqs, status);
	}
}

int main(int argc, char **argv)
{
	int my_rank = -1, size = -1;
	int i, j;
	char **sendv, **recvv;
	MPI_Status *status;
	MPI_Request *reqs;
	long szentry;
	long nentry;
	int src, dest;
	struct timespec start, end;
	double diffusec;

	if (argc == 3) {
		szentry = atoi(argv[1]);
		nentry = atoi(argv[2]);
	} else {
		szentry = SZENTRY_DEFAULT;
		nentry = NENTRY_DEFAULT;
	}

	status = (MPI_Status *)malloc(sizeof(MPI_Status) * nentry);
	reqs = (MPI_Request *)malloc(sizeof(MPI_Request) * nentry);

	int actual;

	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &actual);
	printf("Thread support level is %d\n", actual);

	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	src = (size + my_rank - 1) % size;
	dest = (my_rank + 1) % size;

	printf("rank=%d, size=%d, src=%d, dest=%d\n",
	       my_rank, size, src, dest);

	sendv = malloc(sizeof(char *) * nentry);
	if (!sendv) {
		printf("malloc failed");
		goto fn_fail;
	}
	for (i = 0; i < 1; i++) {
		sendv[i] = (char *)mmap(0, szentry, PROT_READ | PROT_WRITE,
					MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		if (sendv[i] == MAP_FAILED) {
			printf("mmap failed");
			goto fn_fail;
		}
		dprintf("[%d] sendv[%d]=%p\n", my_rank, i, sendv[i]);
		memset(sendv[i], 0xaa, szentry);
	}

	recvv = malloc(sizeof(char *) * nentry);
	if (!recvv) {
		printf("malloc failed");
		goto fn_fail;
	}
	for (i = 0; i < 1; i++) {
		recvv[i] = (char *)mmap(0, szentry, PROT_READ|PROT_WRITE,
					MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
		if (recvv[i] == MAP_FAILED) {
			printf("mmap failed");
			goto fn_fail;
		}
		dprintf("[%d] recvv[%d]=%p\n", my_rank, i, recvv[i]);
		memset(recvv[i], 0, szentry);
	}

	printf("after memset\n");

	print_cpu_last_executed_on("main");

	printf("Before 1st barrier\n"); fflush(stdout);
	MPI_Barrier(MPI_COMM_WORLD);
	if (my_rank == 0) {
		clock_gettime(CLOCK_REALTIME, &start);
	}
	sendrecv(my_rank, nentry, sendv, recvv, szentry, src, dest, reqs,
		 status, 0);
	printf("Before 2nd barrier\n"); fflush(stdout);
	MPI_Barrier(MPI_COMM_WORLD);
	if (my_rank == 0) {
		clock_gettime(CLOCK_REALTIME, &end);
		diffusec = DIFFNSEC(end, start) / (double)1000;
		printf("%4.4f sec\n",
		       DIFFNSEC(end, start) / (double)1000000000);
		fflush(stdout);
	}

 fn_exit:
	MPI_Finalize();
	return 0;
 fn_fail:
	goto fn_exit;
}
