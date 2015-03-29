#include <stdlib.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "gfserver.h"

/**
 * The data struct used to read the data into before passing it on
 * to the client
 */
struct data_struct
{
	char *buffer;
	size_t size;
};

/**
 * Writes the data recieved to memory to be sent later
 * @param void *new_data
 *		Data that was deliverd via the curl handle
 * @param size_t size
 *		The size of that data
 * @param size_t nmemb,
 *		The number of bytes of in the new data
 * @param void *saved_data
 *		The data that has been saved already from previous reads
 * @return
 *		The real size of the data that was saved into memory
 * @note
 *		The real size of the data saved into memory is found from size*nmemb
 */
static size_t write_callback( void *new_data, size_t size, size_t nmemb, void *saved_data )
{
	size_t realsize = size * nmemb;
	struct data_struct *data = (struct data_struct *) saved_data;

	// reserve memory
	data->buffer = realloc( data->buffer, data->size + realsize + 1 );
	if( data->buffer == NULL )
	{
		// no memory
		fprintf( stderr, "not enough memory (realloc returned NULL)\n" );
		return 0;
	}

	// copy new data into memory
	memcpy( &(data->buffer[data->size]), new_data, realsize );
	data->size += realsize;
	data->buffer[data->size] = 0; // adding 0 terminator

	return realsize;
}

/**
 * handle with curl retrieves a file from a server and relays the
 * contents to the client
 * @param gfcontext_t *ctx
 *		Context information for the proxy server, used in
 *		gfs_sendheader and gfs_send
 * @param char *path
 *		The pathe of the requested source
 * @param void* arg
 *		The argument registered for this thread with the GFS_WORKER_ARG option
 * @return
 *		The number of bytes that were trasfered
 */
ssize_t handle_with_curl(gfcontext_t *ctx, char *path, void* arg)
{
	size_t bytes_transferred;
	char buffer[4096];
	char *data_dir = arg;

	strcpy(buffer,data_dir);
	strcat(buffer,path);

	printf( "handle_with_curl: %s\n", buffer );

	// get an easy curl handle
	CURL *handle = curl_easy_init();
	if( !handle )
	{
		fprintf( stderr, "curl handle could not initialize\n" );
		return EXIT_FAILURE;
	}

	// set options for curl
	CURLcode curl_code;
	struct data_struct output;
	output.buffer = NULL;
	output.size = 0;

	curl_easy_setopt( handle, CURLOPT_URL, buffer );
	curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, write_callback );
	curl_easy_setopt( handle, CURLOPT_WRITEDATA, (void *) &output );

	// perform curl, check for success
	curl_code = curl_easy_perform( handle );
	if( curl_code != 0 )
	{
		fprintf( stderr, "curl perform error %d\n", curl_code );
		return EXIT_FAILURE;
	}

	// handle 404 error
	long http_code = 0;
	curl_easy_getinfo( handle, CURLINFO_RESPONSE_CODE, &http_code );
	if( http_code == 404 )
		return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);

	// handle null output
	if( output.buffer == NULL )
		return gfs_sendheader(ctx, GF_FILE_NOT_FOUND, 0);

	// cleanup
	curl_easy_cleanup( handle );

	// send the header
	gfs_sendheader(ctx, GF_OK, output.size);
	
	// since the whole file is in memory, send it
	bytes_transferred = gfs_send(ctx, output.buffer, output.size);

	/* Sending the file contents chunk by chunk. */
	/*
	bytes_transferred = 0;
	while(bytes_transferred < output.size)
	{
		// send the chunk
		write_len = gfs_send(ctx, output.buffer, 4096);

		// total up bytes transfered
		bytes_transferred += write_len;

		// advance the pointer
		output.buffer += write_len;
	}
	*/

	return bytes_transferred;
}

