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
			printf( "usage:\n\twebclient [options]\noptions:\n\t-s server address (Default: 0.0.0.0)\n\t-p port (Default: 8888)\n\t-t number of worker threads (Default: 1, Range: 1-1000)\n\t-w path to workload file (Default: workload.txt)\n\t-d path to downloaded file directory (Default: null)\n\t-r number of total requests (Default: 10, Rangke: 1-1000)\n\t-m path to metric file (Default: metircs.txt)\n\t-h show help message\n" );
			return 0;
		}
	}


	printf("initializing client...\n");
	printf("");
	printf("");
	printf("");
	printf("");
	printf("");
}

