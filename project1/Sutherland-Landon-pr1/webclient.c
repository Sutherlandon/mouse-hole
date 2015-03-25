#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>

#define SOCKET_ERROR	-1
#define BUFFER_SIZE		250
#define QUEUE_SIZE		5
#define MAX_THREADS		100

// struct for worker threads
struct worker {
	int id;
	int num_requests;
	int num_files;
	int total_bytes;
	int response_time;
	char *download_path;
	char *file_names[256];
};

// global variables for all threads
struct sockaddr_in server_address;
struct hostent *server;
struct worker worker_data_array[MAX_THREADS];
int threads_running = 0;
int threads_flag = 0;

// print errors and kill the program
void error( const char* msg )
{
	perror(msg);
	exit(0);
}

// given and int, returns how many digits are in it
int intlen( int i )
{
    int n = 0;
    while( i ) { i /= 10; n++; }
    return n;
}

// takes a string and replaces \n with \0
void stripNewline( char *line )
{
	while( *line != 0 )
	{
		if( *line == '\n' )
			*line = '\0';
		line++;
	}
}

/**
 * Takes a server response and parses it
 * @param const char* respose
 *		A string response from a server request
 * @return
 *		A code based on what was parsed
 *		code: -1, parse error, server response was incorrectly formatted
 *		code: 0, NO_FILE_FOUND, the requested file was not found
 *		code: > 0, the size of the file to be transfered
 */
int readResponse( const char* response )
{
	int code = -1;
	char *token, *rest = strdup( response );

	// check the first token (GetFile)
	if(( token = strsep( &rest, " " )) != NULL )
		if( strcmp( token, "GetFile" ) == 0 ) 
		{
			// check the second token (GET)
			if(( token = strsep( &rest, " " )) != NULL )
			{
				if( strcmp( token, "OK" ) == 0 )
				{
					// get the file size
					code = atoi( rest );
				} else {
					// return file not found code
					if( strcmp( token, "FILE_NOT_FOUND" ) == 0 )
						code = 0;
				}
			}
		}

	return code;
}

/**
 * Perform the getFile protocol
 */
void *getFile( void *worker_data_ptr )
{
	// worker data
	struct worker *worker_data;
	worker_data = (struct worker *) worker_data_ptr;

	// client variables
	int client_sockfd, bytes;
	char *buffer = (char *) malloc( 256 );
	char *file_name = NULL;
	time_t start_time, end_time;

	// server response variables
	int file_size;
	int total_bytes = 0;

	// report to user
	printf( "Worker %d: beginning work\n", worker_data->id );
	start_time = time( NULL );

	// download each file assigned to this worker
	int i;
	for( i = 0; i < worker_data->num_requests; i++ )
	{
		// create the socket
		client_sockfd = socket( AF_INET, SOCK_STREAM, 0 );
		if( client_sockfd == SOCKET_ERROR )
			error( "ERROR could not create socket\n" );

		// connect to the server
		//printf( "connecting to the server...\n" );
		if( connect( client_sockfd, (struct sockaddr *) &server_address, sizeof( server_address )) == SOCKET_ERROR )
			error( "ERROR could not connect to server\n" );

		// get the next file name, if we have already read all the files
		// then the modulo will make it start at the beginning again.
		// this is in the case that there are more requests to be done than
		// files to be read.
		file_name = worker_data->file_names[i % worker_data->num_files];

		// get the message to send
		bzero( buffer, 256 ); // zero out the buffer
		stripNewline( file_name );
		snprintf( buffer, 255, "GetFile GET %s", file_name );
		printf( "Worker %d: %s\n", worker_data->id, buffer );

		// send the file request
		bytes = send( client_sockfd, buffer, strlen( buffer ), 0 );
		if( bytes < 0 )
			error( "ERROR writing to socket\n" );
		//printf( "%d bytes were sent\n", bytes );
		
		// receive the server response
		bzero( buffer, 256 ); // zero out the buffer
		bytes = recv( client_sockfd, buffer, 255, 0 );
		file_size = readResponse( buffer );

		// based on the file_size, read in the file or
		// print the correct response
		if( file_size > 0 )
		{
			// create a new file to write to
			FILE *download_file;
			char *download_file_path = (char *) malloc( 256 );
			int download = 0; // 0 for we have no path and should discard data received

			// if we don't have a download_path set, we can discard any data we receive
			if( worker_data->download_path )
			{
				// 1 for we have a path, and should download
				download = 1;

				// set the file name
				snprintf( download_file_path, 255, "%s%s", worker_data->download_path, file_name );

				// create the file or clear it if it does exist
				download_file = fopen( download_file_path, "wb" );
				fclose( download_file );
			}

			// ask for the first piece of the file
			send( client_sockfd, "continue", strlen("continue"), 0 );

			// process incoming file
			while(( bytes = recv( client_sockfd, buffer, 255, 0 )) > 0 )
			{
				// only save what we receive if a download path was specified
				if( download )
				{
					// open the file to write
					download_file = fopen( download_file_path, "ab" );

					// write to file
					fwrite( buffer, 1, bytes, download_file );
					//printf( "wrote: %s\n", buffer );

					// close file
					fclose( download_file );
				}

				// save they number of bytes received
				total_bytes += bytes;
				
				// ask for next piece
				send( client_sockfd, "continue", strlen("continue"), 0 );
				bzero( buffer, 256 ); // zero out the buffer
			}

			// stop the clock
			end_time = time ( NULL );

			if( download )
				printf( "Worker %d: received: '%s' (%d bytes)\n", worker_data->id, file_name, total_bytes );
			else
				printf( "Worker %d: received: '%s' (%d bytes) NOT SAVED\n", worker_data->id, file_name, total_bytes );
		}
		else if( file_size == 0 )
		{
			// print no file found
			printf( "Server could not find the specified file (FILE_NOT_FOUND)\n" );
		} else {
			// code=-1, print formatting error
			printf( "ERROR incorrect server response format, you may be under attack!\n" );
			printf( "Worker %d: received: '%s'\n", worker_data->id, buffer );
		}

		// save the metrics
		worker_data->total_bytes += total_bytes;
		worker_data->response_time = (int)(end_time-start_time);

		// close the socket
		close( client_sockfd );
	}

	// wait for any other thread to update the count
	while( threads_flag );
	threads_flag = 1; // set the lock
	threads_running -= 1; // decrement the counter
	threads_flag = 0; // release the lock

	// close the connection to the server
	printf( "Worker %d: done, received: %d bytes, response time: %d\n", worker_data->id, worker_data->total_bytes, worker_data->response_time );
	pthread_exit( NULL );
}


int main(int argc, char **argv)
{
	// client settings
	FILE *workload;
	char *file_name = NULL;
	int i, j;
	
	// user input defaults
	int port = 8888;
	int thread_count = 1;
	int requests = 10;
	char *server_host = "0.0.0.0";
	char *workload_path = "workload.txt";
	char *metrics_path = "metrics.txt";
	char *download_path = NULL;
	size_t len = 0;

	// server response
	int response_code;
	int threads_total = 0;
	
	// get arguments from the command line
	for( i = 1; i < argc; i++ )
	{
		if( strcmp( argv[i], "-s" ) == 0 )
			server_host = argv[++i];

		if( strcmp( argv[i], "-p" ) == 0 )
			port = atoi(argv[++i]);
		
		if( strcmp( argv[i], "-t" ) == 0 )
			thread_count = atoi(argv[++i]);
		
		if( strcmp( argv[i], "-r" ) == 0 )
			requests = atoi(argv[++i]);

		if( strcmp( argv[i], "-w" ) == 0 )
			workload_path = argv[++i];

		if( strcmp( argv[i], "-m" ) == 0 )
			metrics_path = argv[++i];

		if( strcmp( argv[i], "-d" ) == 0 )
			download_path = argv[++i];

		if( strcmp( argv[i], "-h" ) == 0 )
		{
			printf( "usage:\n\twebclient [options]\noptions:\n\t-s server address (Default: 0.0.0.0)\n\t-p port (Default: 8888)\n\t-t number of worker threads (Default: 1, Range: 1-100)\n\t-w path to workload file (Default: workload.txt)\n\t-d path to downloaded file directory (Default: null)\n\t-r number of total requests (Default: 10, Range: 1-1000)\n\t-m path to metric file (Default: metircs.txt)\n\t-h show help message\n" );
			return 0;
		}
	}

	// initialize the client
	printf( "initializing client...\n" );
	
	// get the server
	printf( "finding server...\n" );
	server = gethostbyname( server_host );
	if( server == NULL )
	{
		printf( "ERROR no such host %s", server_host );
		return 0;
	}

	// save the server information
	bzero((char *) &server_address, sizeof( server_address )); // zero out the server address
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	// copy the server host address
	bcopy((char *) server->h_addr, (char *) &server_address.sin_addr.s_addr, server->h_length );

	// create the workload, distribute it, and spawn threads!
	pthread_t workers[thread_count];

	printf( "creating workers' dataset...\n" );
	if( !download_path )
		printf( "WARNING No download path specified. Anything recieved will be discarded\n" );

	// create the workers' dataset
	for( i = 0; i < thread_count && i < requests; i++ )
	{
		worker_data_array[i].id = i;
		worker_data_array[i].download_path = download_path;
	}

	// spread out the requests accross all the threads round robin style
	for( i = 0, j = 0; i < requests; i++, j = (j+1) % thread_count )
		worker_data_array[j].num_requests += 1;

	// read the file and distribute the workload round robin style
	// in the case of more threads than files, we will start reading
	// the file from the top again and assign different threads
	// duplicate files to get
	printf( "distributing workload (%s)...\n", workload_path );
	int work_counter = 0;
	i = 0; j = 0;
	while( work_counter < thread_count && work_counter < requests)
	{
		// open workload file
		workload = fopen( workload_path, "r" );
		if( workload == NULL )
			error( "ERROR could not open workload file\n" );

		// assign files to workers
		while( getline( &file_name, &len, workload ) != -1 &&
				work_counter < thread_count && work_counter < requests )
		{
			stripNewline( file_name );
			worker_data_array[i].file_names[j] = strdup( file_name );
			worker_data_array[i++].num_files = j+1;

			// if we have assigned all the workers a file
			// then then go around and assign them all another
			// until all the files have been assigned
			if( i == thread_count ) {
				i = 0; j++;
			}

			// count how many threads get work so we can know if we need to
			// have threads double up on files to get so that every thread
			// created gets to do something
			work_counter++;
		}

		// close the file
		fclose( workload );
	}

	//spawn the threads
	printf( "spawning threads...\n" );
	for( i = 0; i < thread_count && i < requests; i++ )
	{
		// create a new thread
		response_code = pthread_create( &workers[i], NULL, getFile, (void *) &worker_data_array[i] );
		if( response_code )
		{
			printf( "ERROR return code from pthread_create() is %d\n", response_code );
			return -1;
		}

		// add a thread to the running count
		threads_running++;
		threads_total++;
	}

	// wait for all the workers to finish
	while( threads_running != 0 );

	// calculate metrics
	FILE *metrics;
	char *metrics_str = (char *) malloc( 512 );
	int total_bytes_received = 0;
	int total_response_time = 0;
	int average_response_time;
	int average_throughput;

	// totals
	for( i = 0; i < threads_total; i++ )
	{
		total_bytes_received += worker_data_array[i].total_bytes;
		total_response_time += worker_data_array[i].response_time;
	}
	
	// averages
	average_response_time = total_response_time / threads_total;
	average_throughput = total_bytes_received / total_response_time;
	
	// save metrics
	metrics = fopen( metrics_path, "w" );
	if( metrics == NULL )
		error( "ERROR could not open metrics file\n" );

	snprintf( metrics_str, 511, "From workload: %s\n\ttotal bytes received:\t\t%dB\n\taverage response time:\t\t%ds\n\taverage throughput:\t\t%d B/s\n", workload_path, total_bytes_received, average_response_time, average_throughput );
	fwrite( metrics_str, 1, strlen(metrics_str), metrics );
	fclose( metrics );
	printf( "\nThese metrics saved at: %s\n", metrics_path );
	printf( "%s\n", metrics_str );

	pthread_exit( NULL );
	return 0;
}

