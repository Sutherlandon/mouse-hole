#include <stdlib.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "gfserver.h"
#include "shm_channel.h"

#define SOCKET_ERROR -1

// the shared memory segment this thread will use
shm_segment_t shm_segment;

/** 
 * Takes a server response and parses it 
 * @param const char* respose 
 *      A string response from a server request 
 * @return 
 *      A code based on what was parsed 
 *      code: -1, parse error, server response was incorrectly formatted 
 *      code: 0, NO_FILE_FOUND, the requested file was not found 
 *      code: > 0, the size of the file to be transfered 
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
 * connects to the cache and established the shared
 * memory segment
 * @return
 *		The socket file descriptor
 */
int connect_to_cache()
{
	// connect to the cache
	printf( "connecting to cache...\n" );
	int port = 8080;
	char *cache_host = "localhost";
	struct sockaddr_in cache_address;
	struct hostent *cache;
	int proxy_sockfd = -1;
	int i, num_segments, bytes = 0;
	char buffer[255];

	// get the cache and save its info
	cache = gethostbyname( cache_host );
	if( cache == NULL )
	{
		fprintf( stderr, "ERROR no such host %s", cache_host );
		exit(EXIT_FAILURE);
	}
	cache_address.sin_family = AF_INET;
	cache_address.sin_port = htons( port );
	bcopy( (char *) cache->h_addr, (char *) &cache_address.sin_addr.s_addr, cache->h_length );

	// create the socket and connect to the cache
	proxy_sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	if( proxy_sockfd == SOCKET_ERROR )
		fprintf( stderr, "ERROR could not create socket\n" );

	// wait until we connect to the cache
	while( connect( proxy_sockfd, (struct sockaddr *) &cache_address, sizeof( cache_address) == SOCKET_ERROR ))
	{
		// wait 5 seconds and try again.
		printf( "Cache unavailable, trying again in 5 seconds...\n" );
		sleep( 5 );
	}

	// select the next available segment
	shm_segment_t segments[num_segments];
	for( i = 0; ; i = (i+1) % num_segments )
		if( segments[i]->not_used == 1 )
		{
			shm_segment = segments[i];
			shm_segment->not_used--;
			break;
		}

	// send segment identifiers
	snprintf( buffer, 255, "GetFile SEGMENT %d", shm_segment->seg_id );
	bytes = send( proxy_sockfd, buffer, strlen( buffer ), 0 );
	if( bytes == 0 )
		fprintf( stderr, "ERROR failed to send segment id.\n" );

	// wait for ready response
	bzero( buffer, 255 );
	bytes = recv( proxy_sockfd, buffer, 255, 0 );
	if( strcmp( buffer, "GetFile SEGMENT CACHE_READY" ) != 0 )
		fprintf( stderr, "ERROR caches failed to attach to segments.\n" );

	return proxy_sockfd;
}

/**
 * handles clean up of shared memory and sockets
 */
void disconnect_from_cache( int proxy_sockfd )
{
	// reset the shared memory segment for later use
	bzero( shm_segment->data, strlen( shm_segment->data ));
	shm_segment->not_used = 1;
	shm_segment->sem = 1;

	// close the socket
	close( proxy_sockfd );
}

/**
 * Handles requests with a cache
 */
ssize_t handle_with_cache(gfcontext_t *ctx, char *path, void* arg)
{
	int proxy_sockfd, bytes;
	size_t file_len, bytes_transferred;
	ssize_t write_len;
	char buffer[4096];
	char *data_dir = arg;

	strcpy(buffer,data_dir);
	strcat(buffer,path);

	// connect to the cache
	proxy_sockfd = connect_to_cache();
	if( proxy_sockfd == -1 )
		return EXIT_FAILURE;
		
	// request the file
	snprintf( buffer, 255, "GetFile GET %s", path );
	bytes = send( proxy_sockfd, buffer, strlen( buffer ), 0 );
	if( bytes == 0 )
		fprintf( stderr, "ERROR failed to send path.\n" );
	
	// recieve file header
	bzero( buffer, 255 );
	bytes = recv( proxy_sockfd, buffer, 255, 0 );
	file_len = readResponse( buffer );
	switch( file_len )
	{
		case -1:
			fprintf( stderr, "Parse error, the cache did not respond properly" );
			return EXIT_FAILURE;
		case 0:
			return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);
		default:
			// send header to client
			gfs_sendheader(ctx, GF_OK, file_len);
	}

	// read the file in chunks from shared memory and
	// send it to the client
	bytes_transferred = 0;
	while( bytes_transferred < file_len )
	{
		// wait for the next piece
		while( !shm_segment->sem );
		shm_segment->sem--; // block writing

		// send the next chunk of the file
		write_len = gfs_send(ctx, shm_segment->data, strlen( shm_segment->data ));

		shm_segment->sem++; // unblock writing
		bytes_transferred += write_len;
	}

	// cleanup shared memory and sockets
	disconnect_from_cache( proxy_sockfd );

	return bytes_transferred;
}

