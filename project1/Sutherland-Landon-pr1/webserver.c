#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define SOCKET_ERROR	-1
#define BUFFER_SIZE		250
#define QUEUE_SIZE		5
#define MAX_THREADS		1000

// struct for worker threads
struct worker {
	int id;
	int working;
	int sockfd;
	char *path_to_files;
};

// global variables for all threads
struct worker worker_data_array[MAX_THREADS];

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
void strReplace( char *line, char a, char b )
{
    while( *line != 0 )
    {
        if( *line == a )
            *line = b;
        line++;
    }
}

/**
 * Get the name of a file to server from a request
 * @param const char* request
 *		Format: GetFile GET <filename>
 * @return
 *		char* <filname>
 *		NULL if there was an error in the format
 */
char *getFileName( const char* request )
{
	char *file_name = NULL;
	char *token, *rest = strdup(request);
	
	// check the first token (GetFile)
	if(( token = strsep( &rest, " " )) != NULL )
		if( strcmp( token, "GetFile" ) == 0 )
		{
			// check the second token (GET)
			if(( token = strsep( &rest, " " )) != NULL )
				if( strcmp( token, "GET" ) == 0 )
				{
					// take the third token as the file name
					if(( token = strsep( &rest, " " )) != NULL )
						file_name = strdup( token );
				}
		}

	// return the file_name, or null if none was found
	return file_name;
}

/**
 * Perform the getFile protocol
 */
void *getFile( void *worker_data_ptr )
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

	printf( "Worker %d beginning work.\n", worker_data->id );
	bzero( buffer, 256); // zero out the buffer

	// get the client request
	if( recv( worker_data->sockfd, buffer, 255, 0 ) == SOCKET_ERROR )
		error( "ERROR reading from client socket\n" );
	printf( "client request: %s\n", buffer );

	// extract the file name, add it to the path, and open it
	snprintf( file_name, 255, "%s%s", worker_data->path_to_files, getFileName( buffer ));
	printf( "fetching file: %s\n", file_name );
	file_to_serve = fopen( file_name, "rb" );

	if( file_to_serve != NULL )
	{
		int total_bytes = 0;

		// get the file size
		fseek( file_to_serve, 0L, SEEK_END );
		file_size = ftell( file_to_serve );
		fseek( file_to_serve, 0L, SEEK_SET );
		printf( "file size: %d\n", file_size );

		// send the header and the first byte of the file
		bzero( buffer, 256); // zero out the buffer
		bzero( reply, 256); // zero out the buffer
		if(( bytes_read = fread( buffer, 1, 1, file_to_serve )) != 0 )
		{
			// format the reply
			snprintf( reply, 255, "%s %d", get_file_ok, file_size );
			strReplace( reply, '\0', ' ' );
			reply_size = 3 + sizeof( get_file_ok ) + intlen( file_size );
			printf( "reply size: %d\n", reply_size );

			// write each piece of the file to the socket
			if(( bytes = send( worker_data->sockfd, reply, reply_size, 0 )) == SOCKET_ERROR )
				error( "ERROR writing to client socket\n" );
			printf( "sent: '%s' (%d bytes)\n", reply, bytes );

			// wait for client to ask for the next piece
			recv( worker_data->sockfd, buffer, strlen("continue"), 0 );

			bzero( buffer, 256); // zero out the buffer
			bzero( reply, 256); // zero out the buffer
			fseek( file_to_serve, 0L, SEEK_SET ); // reset file pointer to the beginning
		}

		// send the rest of the file 128 bytes at a time
		while(( bytes_read = fread( buffer, 1, 128, file_to_serve )) != 0 )
		{
			// write each piece of the file to the socket
			if(( bytes = send( worker_data->sockfd, buffer, bytes_read, 0 )) == SOCKET_ERROR )
				error( "ERROR writing to client socket\n" );
			//printf( "sent: %d bytes\n", bytes );
			total_bytes += bytes;

			// wait for client to ask for the next piece
			recv( worker_data->sockfd, buffer, strlen("continue"), 0 );

			bzero( buffer, 256); // zero out the buffer
			bzero( reply, 256); // zero out the buffer
		}

		//printf( "bytes left: %ld\n", file_size - ftell( file_to_serve ));

		// reading in 128 byte chunks we might miss the last few
		// if file_size % 128 != 0 So get the last few
		int i = 0;
		bytes_read = 0;
		while( fread( &buffer[i++], 1, 1, file_to_serve ) == 1 )
			bytes_read++;
		//printf( "%d, %d", i, bytes_read );

		// send the last bit of the file
		if( bytes_read > 0 )
		{
			// write the last piece of the file to the socket
			if(( bytes = send( worker_data->sockfd, buffer, bytes_read, 0 )) == SOCKET_ERROR )
				error( "ERROR writing to client socket\n" );
			//printf( "sent: %d bytes\n", bytes );
			total_bytes += bytes;

			bzero( buffer, 256); // zero out the buffer
			bzero( reply, 256); // zero out the buffer
		}

		printf( "total bytes sent: %d\n", total_bytes );

		// close the file
		fclose( file_to_serve );
	} else {
		// NO file found, send reply
		if( send( worker_data->sockfd, get_file_not_found, strlen( get_file_not_found ), 0 ) == SOCKET_ERROR )
			error( "ERROR writing to client socket\n" );
		printf( "GetFile FILE_NOT_FOUND reply sent\n" );
	}

	// close this client, free the worker, kill the thread
	close( worker_data->sockfd );
	printf( "Worker %d done.\n", worker_data->id );
	worker_data->working = 0;
	pthread_exit( NULL );
}

int main(int argc, char **argv)
{
	// initialize the server
	// server settings
	int server_sockfd, client_sockfd;
	struct sockaddr_in server_address, client_address;
	socklen_t client_size = sizeof( client_address );

	// user input defaults
	char *path_to_files = ".";
	int port = 8888;
	int thread_count = 1;

	int i, response_code;
	
	// get arguments from the command line
	for( i = 1; i < argc; i++ )
	{
		if( strcmp( argv[i], "-p" ) == 0 )
			port = atoi(argv[++i]);
		
		if( strcmp( argv[i], "-t" ) == 0 )
			thread_count = atoi(argv[++i]);

		if( strcmp( argv[i], "-f" ) == 0 )
			path_to_files = argv[++i];

		if( strcmp( argv[i], "-h" ) == 0 )
		{
			printf( "usage:\n\twebserver [options]\noptions:\n\t-p port (Default: 8888)\n\t-t number of worker threads (Default: 1, Range: 1-1000)\n\t-f path to static files (Default: .)\n\t-h show help message\n" );
			return 0;
		}
	}
	printf( "initialzing server...\n" );
	printf( "port %d, threads %d, path %s\n", port, thread_count, path_to_files );

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
	pthread_t workers[thread_count];
	for( i = 0; i < thread_count; i++ )
	{
		worker_data_array[i].id = i;
		worker_data_array[i].path_to_files = path_to_files;
	}

	// listen for incoming clients and accept them
	printf( "listening...\n" );
	i = 0;
	do {
		// wait for the next client to connect
		client_sockfd = accept( server_sockfd, (struct sockaddr *) &client_address, &client_size );
		if( client_sockfd == SOCKET_ERROR )
			error( "ERROR accepting client\n" );

		// assign the new client to the next available thread
		// find the next available worker
		for( ; worker_data_array[i].working == 1; i = (i+1) % thread_count );

		// assign the client to it
		worker_data_array[i].working = 1;
		worker_data_array[i].sockfd = client_sockfd;

		// spawn the new thread
		// create a new thread
		response_code = pthread_create( &workers[i], NULL, getFile, (void *) &worker_data_array[i] );
		if( response_code )
		{
			worker_data_array[i].working = 0;
			printf( "ERROR return code from pthread_create(%i) is %d\n", i, response_code );
		}

	} while( 1 ); // always listen for new clients

	// close the sockets
	close( server_sockfd );
	pthread_exit( NULL );
	return 0;
}

