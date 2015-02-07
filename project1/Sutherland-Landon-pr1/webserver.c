#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SOCKET_ERROR	-1
#define BUFFER_SIZE		250
#define QUEUE_SIZE		5

void error( const char* msg )
{
	perror(msg);
	exit(0);
}

int main(int argc, char **argv)
{
	// initialize the server
	// server settings
	int server_sockfd, client_sockfd;
	struct sockaddr_in server_address, client_address;
	socklen_t client_size = sizeof( client_address );
	char buffer[256];

	// user input defaults
	char* path=".";
	int port = 8888;
	int thread_count = 1;
	
	// get arguments from the command line
	int i;
	for( i = 1; i < argc; i++ )
	{
		if( strcmp( argv[i], "-p" ) == 0 )
			port = atoi(argv[++i]);
		
		if( strcmp( argv[i], "-t" ) == 0 )
			thread_count = atoi(argv[++i]);

		if( strcmp( argv[i], "-f" ) == 0 )
			path = argv[++i];

		if( strcmp( argv[i], "-h" ) == 0 )
		{
			printf( "usage:\n\twebserver [options]\noptions:\n\t-p port (Default: 8888)\n\t-t number of worker threads (Default: 1, Range: 1-1000)\n\t-f pather to static files (Default: .)\n\t-h show help message\n" );
			return 0;
		}
	}
	printf("port %d, threads %d, path %s\n", port, thread_count, path);
	printf( "initialzing server...\n" );

	// create the socket
	printf("making socket...\n");
	server_sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if( server_sockfd == SOCKET_ERROR )
		error("ERROR opening socket\n");

	// connect to the host
	printf("connecting to host on port %d...\n", port);
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port);
	if( bind( server_sockfd, (struct sockaddr *) &server_address, sizeof( server_address )) == SOCKET_ERROR )
		error("ERROR on binding");

	// create the queue for listening, up to 5 connections may be waiting to connect to the server at a time
	printf("making listening queue...\n");
	listen( server_sockfd, QUEUE_SIZE );

	// listen for incoming clients and accept them
	printf("listening...\n");
	client_sockfd = accept( server_sockfd, (struct sockaddr *) &client_address, &client_size );
	if( client_sockfd == SOCKET_ERROR )
		error("ERROR accepting client\n");

	printf("client connected\n");
	bzero( buffer, 256); // zero out the buffer

	// read the client's message
	if( read( client_sockfd, buffer, 255 ) == SOCKET_ERROR )
		error( "ERROR reading from client socket\n" );
	printf( "Client says: %s\n", buffer );

	// send a reply
	if( write( client_sockfd, "message recieved", 16 ) == SOCKET_ERROR )
		error( "ERROR writing to client socket\n" );
	printf( "confirmation sent\n" );

	// close the sockets
	close( client_sockfd );
	close( server_sockfd );
	return 0;
}

