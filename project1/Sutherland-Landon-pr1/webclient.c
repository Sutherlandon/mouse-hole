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

#define SOCKET_ERROR	-1
#define BUFFER_SIZE		250
#define QUEUE_SIZE		5
#define MAX_THREADS		1000

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

	// server response variables
	int file_size;
	int total_bytes = 0;

	// report to user
	printf( "Worker %d beginning work\n", worker_data->id );

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
		printf( "%s\n", buffer );

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

			// set the file name
			snprintf( download_file_path, 255, "%s%s", worker_data->download_path, file_name );

			// create the file or clear it if it does exist
			download_file = fopen( download_file_path, "wb" );
			fclose( download_file );

			// ask for the first piece of the file
			send( client_sockfd, "continue", strlen("continue"), 0 );

			// process incoming file
			while(( bytes = recv( client_sockfd, buffer, 255, 0 )) > 0 /*&& total_bytes <= file_size*/ )
			{
				// open the file to write
				download_file = fopen( download_file_path, "ab" );

				// write to file
				fwrite( buffer, 1, bytes, download_file );
				total_bytes += bytes;
				//printf( "wrote: %s\n", buffer );

				// close file
				fclose( download_file );
				
				// ask for next piece
				send( client_sockfd, "continue", strlen("continue"), 0 );
				bzero( buffer, 256 ); // zero out the buffer
			}

			printf( "received: '%s' (%d bytes)\n", file_name, total_bytes );
			//printf( "DONE file_name: %s (%d bytes)\n", file_name, total_bytes );
		}
		else if( file_size == 0 )
		{
			// print no file found
			printf( "Server could not find the specified file (FILE_NOT_FOUND)\n" );
		} else {
			// code=-1, print formatting error
			printf( "ERROR incorrect server response format, you may be under attack!\n" );
			printf( "received: '%s'\n", buffer );
		}

		//printf( "DONE filename: %s, bytes: %d\n", file_name, total_bytes );

		// save the metrics
		worker_data->total_bytes += total_bytes;
		worker_data->response_time = 12;

		// close the socket
		close( client_sockfd );
	}

	// close the connection to the server
	printf( "Worker %d: done, bytes received: %d\n", worker_data->id, worker_data->total_bytes );
	pthread_exit( NULL );
}


int main(int argc, char **argv)
{
	// client settings
	FILE *workload, *metrics;
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
	int total_bytes_recieved = 0;
	
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

	// open workload file
	printf( "opening workload: %s\n", workload_path );
	workload = fopen( workload_path, "r" );
	if( workload == NULL )
		error( "ERROR could not open workload file\n" );

	// create the workload, distribute it, and spawn threads!
	pthread_t workers[thread_count];

	printf( "creating workers' dataset...\n" );
	// create the workers' dataset
	for( i = 0; i < thread_count; i++ )
	{
		worker_data_array[i].id = i;
		worker_data_array[i].download_path = download_path;
		worker_data_array[i].num_requests = requests / thread_count;
	}

	// read the file and distribute the workload round robin style
	printf( "distributing workload...\n" );
	i = 0; j = 0;
	while( getline( &file_name, &len, workload ) != -1 )
	{
		stripNewline( file_name );
		worker_data_array[i].file_names[j] = strdup( file_name );
		worker_data_array[i++].num_files = j+1;

		if( i == thread_count ) {
			i = 0; j++;
		}
	}

	//spawn the threads
	printf( "spawning threads...\n" );
	for( i = 0; i < thread_count; i++ )
	{
		// create a new thread
		response_code = pthread_create( &workers[i], NULL, getFile, (void *) &worker_data_array[i] );
		if( response_code )
		{
			printf( "ERROR return code from pthread_create() is %d\n", response_code );
			return -1;
		}
	}

	// calculate and save metrics
	//for( i = 0; i < thread_count; i++ )
	//	total_bytes_received += workers[i].total_bytes;

	// close the file
	fclose( workload );
	pthread_exit( NULL );

	//fflush(stdout);
}

