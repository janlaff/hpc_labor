#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#define TRYS 5000000

static int throw ()
{
	double x, y;
	x = (double) rand() / (double) RAND_MAX;
	y = (double) rand() / (double) RAND_MAX;
	if ((x *x + y *y) <= 1.0) return 1;

	return 0;
}

int main(int argc, char **argv)
{
	int globalCount = 0, globalSamples = TRYS;
	int *hits;
	int tid;

	#pragma omp parallel shared(hits) private(tid)
	{
		tid = omp_get_thread_num();

		#pragma omp master
		{
			hits = (int*)malloc(omp_get_num_threads() * sizeof(int));
		}

		#pragma omp barrier

		#pragma omp for reduction(+:globalCount)
		for (int i = 0; i < globalSamples; ++i)
		{
			#pragma omp critical
			globalCount += throw();
			hits[tid] = globalCount;
		}

		printf("Thread %d: treffer %d\n", tid, hits[tid]);
	}

	free(hits);

	double pi = 4.0 *(double) globalCount / (double)(globalSamples);

	printf("pi is %.9lf\n", pi);

	return 0;
}