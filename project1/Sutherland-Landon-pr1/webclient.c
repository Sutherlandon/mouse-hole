#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SOCKET_ERROR	-1
#define BUFFER_SIZE		250
#define QUEUE_SIZE		5

// print errors and kill the program
void error( const char* msg )
{
	perror(msg);
	exit(0);
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

// given and int, returns how many digits are in it
int intlen( int i )
{
    int n = 0;
    while( i ) { i /= 10; n++; }
    return n;
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
	printf( "%s\n", rest );

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

int main(int argc, char **argv)
{
	// client settings
	int client_sockfd, bytes;
	struct sockaddr_in server_address;
	struct hostent *server;
	char *buffer = (char *) malloc( 256 );
	char *file_name = NULL;
	size_t len = 0;
	
	// user input defaults
	int port = 8888;
	int thread_count = 1;
	int requests = 10;
	char *server_host = "0.0.0.0";
	char *workload_path = "workload.txt";
	FILE *workload;
	char *metrics_path = "metrics.txt";
	FILE *metrics;
	char *download_path = NULL;

	// server response
	char *file_bytes = (char *) malloc( 256 );
	int file_size, response_code;
	
	// get arguments from the command line
	int i;
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
	// zero out the server address
	bzero((char *) &server_address, sizeof( server_address ));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	// copy the server host address
	bcopy((char *) server->h_addr, (char *) &server_address.sin_addr.s_addr, server->h_length );

	// open workload file
	printf( "opening workload: %s\n", workload_path );
	workload = fopen( workload_path, "r" );
	if( workload == NULL )
		error( "ERROR could not open workload file\n" );

	// read the file and request the files
	while(( bytes = getline( &file_name, &len, workload )) != -1 )
	{
		// create the socket
		client_sockfd = socket( AF_INET, SOCK_STREAM, 0 );
		if( client_sockfd == SOCKET_ERROR )
			error( "ERROR could not create socket\n" );

		// connect to the server
		printf( "connecting to the server...\n" );
		if( connect( client_sockfd, (struct sockaddr *) &server_address, sizeof( server_address )) == SOCKET_ERROR )
			error( "ERROR could not connect to server\n" );
	
		// get the message to send
		bzero( buffer, 256 ); // zero out the buffer
		stripNewline( file_name );
		snprintf( buffer, 255, "GetFile GET %s", file_name );
		printf( "%s\n", buffer );

		// send the file request
		bytes = send( client_sockfd, buffer, strlen( buffer ), 0 );
		if( bytes < 0 )
			error( "ERROR writing to socket\n" );
		printf( "%d bytes were sent\n", bytes );
		
		// receive the server response
		bzero( buffer, 256 ); // zero out the buffer
		bytes = recv( client_sockfd, buffer, 255, 0 );
		printf( "received: '%s' (%d bytes)\n", buffer, bytes );
		file_size = readResponse( buffer );

		// based on the file_size, read in the file or
		// print the correct response
		if( file_size > 0 )
		{
			// create a new file to write to
			FILE *download_file;
			char *download_file_path = (char *) malloc( 256 );
			int total_bytes = 0;

			// set the file name
			snprintf( download_file_path, 255, "%s%s", download_path, file_name );

			// create the file or erase it if it does exist
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
			printf( "total bytes received: %d\n", total_bytes );
		}
		else if( file_size == 0 )
		{
			// print no file found
			printf( "Server could not find the specified file (FILE_NOT_FOUND)\n" );
		} else {
			// code=-1, print formatting error
			printf( "ERROR incorrect server respose format, you may be under attack!\n" );
		}

		// close the connection to the server
		printf( "closing the socket\n-------------------------------------------\n" );
		close( client_sockfd );
	}

	// close the file
	fclose( workload );

	return 0;

	//fflush(stdout);
}

