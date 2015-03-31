#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h> 
#include "shm_channel.h"
#include "simplecache.h"

#define MAX_CACHE_REQUEST_LEN 256
#define SOCKET_ERROR    -1
#define BUFFER_SIZE     250
#define QUEUE_SIZE      5
#define MAX_THREADS     1000

// struct for worker threads
struct worker {
	int id;
	int working;
	int sockfd;
	int shm_seg_id;
	shm_segment_t shm_segment;
};

// global variables for all threads
struct worker worker_data_array[MAX_THREADS];

/**
 * Get the name of a file to server from a request
 * @param const char* request
 *      Format: GetFile GET <filename>
 * @return
 *      char* <filname>
 *      NULL if there was an error in the format
 */
char *getKey( const char* request )
{
	char *key = NULL;
	char *token, *rest = strdup(request);

	// check the first token (GetFile)
	if(( token = strsep( &rest, " " )) != NULL )
		if( strcmp( token, "GetFile" ) == 0 )
		{
			// check the second token (GET)
			if(( token = strsep( &rest, " " )) != NULL )
				if( strcmp( token, "GET" ) == 0 )
				{
					// take the third token as the key
					if(( token = strsep( &rest, " " )) != NULL )
						key = strdup( token );
				}
		}

	// return the key, or null if none was found
	return key;
}

/**
 * Get the id of a segment of shared memory
 * @param const char* request
 *      Format: GetFile SEGMENT <id>
 * @return
 *      int <id>
 *      NULL if there was an error in the format
 */
int getSegmentId( const char* request )
{
	char *id = NULL;
	char *token, *rest = strdup(request);

	// check the first token (GetFile)
	if(( token = strsep( &rest, " " )) != NULL )
		if( strcmp( token, "GetFile" ) == 0 )
		{
			// check the second token (GET)
			if(( token = strsep( &rest, " " )) != NULL )
				if( strcmp( token, "SEGMENT" ) == 0 )
				{
					// take the third token as the id
					if(( token = strsep( &rest, " " )) != NULL )
						id = atoi( token );
				}
		}

	// return the id, or null if none was found
	return id;
}


static void _sig_handler(int signo){
	if (signo == SIGINT || signo == SIGTERM){
		/* Unlink IPC mechanisms here*/
		//for( i=0; i < num_segments; i++ )
		//	shmdt( segments[i]->seg_id );
		close( server_sockfd );
		pthread_exit( NULL );
		simplecache_destroy();
		exit(signo);
	}
}

#define USAGE                                                                 \
	"usage:\n"                                                                    \
"  simplecached [options]\n"                                                  \
"options:\n"                                                                  \
"  -t [thread_count]   Num worker threads (Default: 1, Range: 1-1000)\n"      \
"  -c [cachedir]       Path to static files (Default: ./)\n"                  \
"  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
	{"nthreads",           required_argument,      NULL,           't'},
	{"cachedir",           required_argument,      NULL,           'c'},
	{"help",               no_argument,            NULL,           'h'},
	{NULL,                 0,                      NULL,             0}
};

void Usage() {
	fprintf(stdout, "%s", USAGE);
}

/**
 * See if we have the file in the cache, then send it.
 * otherwise, reply with FILE_NOT_FOUND
 */
void *getFile( void *workder_data_ptr )
{
    // worker_data
    struct worker *worker_data;
    worker_data = (struct worker *) worker_data_ptr;

    // serving variables
    const char *get_file_ok = "GetFile OK";
    const char *get_file_not_found = "GetFile FILE_NOT_FOUND 0 0";
    FILE *file_to_serve;
    char *file_name = (char *) malloc( 256 );
    char *buffer = (char *) malloc( 256 );
    char *reply = (char *) malloc( 256 );
    int file_size = 0, reply_size = 0;
    int bytes_read = 0, bytes = 0;
	int segment_id, file;

	// get the segment id and attach the shared memory
    if( recv( worker_data->sockfd, buffer, 255, 0 ) == SOCKET_ERROR )
        fprintf( stderr, "ERROR reading from proxy socket\n" );
	seg_id = getSegmentId( buffer );
	worker_data->shm_seg_id = seg_id;
	worker_data->shm_segment = (shm_segment_t) shmat( seg_id, (void *) 0, 0 );
	if( worker_data->shm_segment != -1 )
	{
		strcpy( buffer, "GetFile SEGMENT CACHE_READY" );
		send( worker_data->sockfd, buffer, strlen( buffer ), 0 );
	} else {
		fprintf( stderr, "ERROR could not attach shared memory" );
		strcpy( buffer, "GetFile SEGMENT CACHE_FAILED" );
		send( worker_data->sockfd, buffer, strlen( buffer ), 0 );
		return -1;
	}

	// handle GetFile request
	bzero( buffer, 255 );
	bytes = recv( worker_data->sockfd, buffer, 255, 0 );
	file = simplecach_get( getKey( buffer ));

	// read the file and send it via shared memory
	if( file != -1 )
	{
		// calculate file size
		file_size = lseek( file, 0 SEEK_END );
		lseek( file, 0, SEEK_SET );

		// send the file size
		snprintf( buffer, 255, "%s %d", get_file_ok, file_size );
		send( worker_data->sockfd, buffer, strlen( buffer ), 0 );

		// send the file in chunks
		bytes_transferred = 0;
		while( bytes_transferred < file_size )
		{
			while( !workder_data->shm_segment->sem );
			worker_data->shm_segment->sem--; // block reading

			// read the next chunk of the file
			bytes = pread( file, worker_data->shm_segment->data, 255, bytes_transferred );

			worker_data->shm_segment->sem++; // unblock reading
			bytes_transferred += bytes;
		}
	} else {
		printf( "GetFile FILE_NOT_FOUND reply sent" );
		send( worker_data->sockfd, get_file_not_found, strlen( get_file_not_found ), 0 );
	}
	
}

int main(int argc, char **argv)
{
	int nthreads = 1;
	int i;
	char *cachedir = "locals.txt";
	char option_char;

	while ((option_char = getopt_long(argc, argv, "t:c:h", gLongOptions, NULL)) != -1) {
		switch (option_char) {
			case 't': // thread-count
				nthreads = atoi(optarg);
				break;
			case 'c': //cache directory
				cachedir = optarg;
				break;
			case 'h': // help
				Usage();
				exit(0);
				break;
			default:
				Usage();
				exit(1);
		}
	}

	if (signal(SIGINT, _sig_handler) == SIG_ERR){
		fprintf(stderr,"Can't catch SIGINT...exiting.\n");
		exit(EXIT_FAILURE);
	}

	if (signal(SIGTERM, _sig_handler) == SIG_ERR){
		fprintf(stderr,"Can't catch SIGTERM...exiting.\n");
		exit(EXIT_FAILURE);
	}

	/* Initializing the cache */
	simplecache_init(cachedir);

	// connect to the proxy
	int server_sockfd, client_sockfd;
	struct sockaddr_in server_address, client_address;
	socklen_t client_size = sizeof( client_address );
	int i, response_code;

	// create the socket
	printf( "making socket...\n" );
	server_sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if( server_sockfd == SOCKET_ERROR )
		error( "ERROR opening socket\n" );

	// connect to the host
	printf( "connecting to host on port %d...\n", port );
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons( port );
	if( bind( server_sockfd, (struct sockaddr *) &server_address, sizeof( server_address )) == SOCKET_ERROR )
		error( "ERROR connecting to host" );

	// create the queue for listening
	// up to 5 connections may be waiting to connect to the server at a time
	listen( server_sockfd, QUEUE_SIZE );

	// create threads
	pthread_t workers[nthreads];
	for( i = 0; i < nthreads; i++ )
		worker_data_array[i].id = i;

	i = 0;
	do {
		// wait for the proxy to connect
		client_sockfd = accept( server_sockfd, (struct sockaddr *) &client_address, &client_size );
		if( client_sockfd == SOCKET_ERROR )
			error( "ERROR accepting client\n" );

		// assign the new request to the next available thread
		// find the next available worker
		for( ; worker_data_array[i].working == 1; i = (i+1) % thread_count );

		// assign the client to it
		worker_data_array[i].working = 1;
		worker_data_array[i].sockfd = client_sockfd;

		// spawn the new thread
		response_code = pthread_create( &workers[i], NULL, getFile, (void *) &worker_data_array[i] );
		if( response_code )
		{
			worker_data_array[i].working = 0;
			printf( "ERROR return code from pthread_create(%i) is %d\n", i, response_code );
		}

	} while( 1 ); // always listen for new requests

	// close the sockets
	close( server_sockfd );
	pthread_exit( NULL );
	simplecache_destroy();
	return 0;
}
