#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NUM_THREADS 5

void *PrintHello( void *thread_id )
{
	int *id_ptr = (int *) thread_id;
	int tid = *id_ptr;
	printf( "Hello World! From thread #%d!\n", tid );
	pthread_exit( NULL );
}

int main( int argc, char *argv[] )
{
	pthread_t threads[NUM_THREADS];
	long *taskids[NUM_THREADS];
	int code;
	long t;

	// launch threads
	for( t = 0; t < NUM_THREADS; t++ )
	{
		taskids[t] = (long *) malloc(sizeof(long));
		*taskids[t] = t;

		// create a new thread
		printf( "creating thread %ld\n", t);
		if( code = pthread_create( &threads[t], NULL, PrintHello, (void *) taskids[t] ))
		{
			printf( "ERROR return code from pthread_creat() is %d\n", code );
			return -1;
		}
	}

	// last thing main should do
	pthread_exit( NULL );
}
