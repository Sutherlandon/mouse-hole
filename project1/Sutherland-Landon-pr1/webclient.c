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

int main(int argc, char **argv)
{
	// client settings
	int client_sockfd, bytes;
	struct sockaddr_in server_address;
	struct hostent* server;
	char buffer[256];
	
	// user input defaults
	int port = 8888;
	int thread_count = 1;
	int requests = 10;
	char* server_host="0.0.0.0";
	char* workload_path="workload.txt";
	char* metrics_path="metrics.txt";
	char* download_path=NULL;
	
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
			printf( "usage:\n\twebclient [options]\noptions:\n\t-s server address (Default: 0.0.0.0)\n\t-p port (Default: 8888)\n\t-t number of worker threads (Default: 1, Range: 1-100)\n\t-w path to workload file (Default: workload.txt)\n\t-d path to downloaded file directory (Default: null)\n\t-r number of total requests (Default: 10, Rangke: 1-1000)\n\t-m path to metric file (Default: metircs.txt)\n\t-h show help message\n" );
			return 0;
		}
	}

	// initialize the client
	printf( "initializing client...\n" );

	// create the socket
	printf( "creating socket...\n" );
	client_sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if( client_sockfd == SOCKET_ERROR )
		error( "Could not create socket\n" );
	
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
	bzero((char*) &server_address, sizeof( server_address )); 
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	// copy the server host address
	bcopy((char*) server->h_addr, (char*) &server_address.sin_addr.s_addr, server->h_length );

	// connect to the server
	printf( "connecting to the server...\n" );
	if( connect( client_sockfd, (struct sockaddr *) &server_address, sizeof( server_address )) == SOCKET_ERROR )
		error( "ERROR could not connect to server\n" );

	// get messages to send
	do {
		// get the message to send
		printf( "Please enter a message: " );
		bzero( buffer, 256 );
		fgets( buffer, 255, stdin );
	
		// use the command 'exit' to exit the client and stop sending messages
		if( strcmp( buffer, "exit" ) == 10 )
			break;

		// write the message
		bytes = write( client_sockfd, buffer, strlen( buffer ));
		if( bytes < 0 )
			error( "ERROR writing to socket\n" );
		printf( "%d bytes were sent\n", bytes );
		
		bzero( buffer, 256 );
		bytes = read( client_sockfd, buffer, 255 );
		if( bytes < 0 )
			error( "ERROR reading from socket\n" );
		printf( "%s\n", buffer );
		printf( "%d bytes were received\n", bytes );

	} while( 1 ); // always ask for another input

	// close the socket and leave the program
	printf( "closing the socket.\n" );
	close( client_sockfd );
	return 0;
}

