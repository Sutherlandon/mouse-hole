#include "minifyjpeg_xdr.c"
#include "minifyjpeg_clnt.c"

/**
 * Creates a jpeg struct and returns it.
 */
jpeg create_image( void *src_val, size_t src_len )
{
	jpeg *image = (jpeg *) malloc( sizeof( jpeg ));
	image->image.image_len = src_len;
	image->image.image_val = (char *) src_val;
	return *(image);
}

/**
 * Connects to the rpc server and returns
 * the client pointer
 */
CLIENT *get_minify_client( char *server )
{
	CLIENT *client;

	// create the client handle
	if(( client = clnt_create( server, MINIFY_PROG, MINIFY_VERS, "tcp")) == NULL )
	{
		clnt_pcreateerror( "clnt_create() failed\n" );
		exit(1);
	}

	return client;
}

/**
 * Use RPC to obtain a minified version of the jpeg image
 * stored in the array src_val and having src_len bytes.
 */
void *minify_via_rpc( CLIENT* client, void* src_val, size_t src_len, size_t *dst_len )
{
	jpeg orig_image = create_image( src_val, src_len );
	jpeg *small_image; // = (jpeg *) malloc( sizeof( jpeg ));

	if(( minify_jpeg_1( orig_image, small_image, client )) != RPC_SUCCESS )
	{
		clnt_perror( client, "minify_jpeg_1() failed\n" );
		exit(1);
	}

	dst_len = (size_t *) &(small_image->image.image_len);
	return (void *) small_image->image.image_val;
}

