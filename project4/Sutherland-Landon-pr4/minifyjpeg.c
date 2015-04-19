#include "minifyjpeg.h"
#include "magickminify.h"
/* Implement the needed server-side functions here */

bool_t minify_jpeg_1_svc( jpeg orig_image, jpeg *small_image, struct svc_req *rqstp)
{
	bool_t retval = 0;
	small_image = (jpeg *) malloc( sizeof( jpeg ));

	printf( "minify_init()... \n" );
	fflush( stdout );

	// initialize the library
	magickminify_init();

	printf( "minifying image...\n" );
	fflush( stdout );

	// minify the image and return the length parameter
	small_image->image.image_val =
		(char *) magickminify( (void *) orig_image.image.image_val,
							   (ssize_t) orig_image.image.image_len,
							   (ssize_t *) &(small_image->image.image_len));

	printf( "writing to file...\n" );
	fflush( stdout );

	// write to file to test minify
	FILE *image_file;
	image_file = fopen( "svc_image.jpg", "w" );
	fwrite( small_image->image.image_val, 1, small_image->image.image_len, image_file );
	fclose( image_file );

	// cleanup the library
	magickminify_cleanup();

	printf( "done\n" );
	fflush( stdout );

	return retval;
}

int minify_prog_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	printf( "Freeing results..." );
	fflush( stdout );

	xdr_free( xdr_result, result );
	
	printf( "done\n" );
	fflush( stdout );

	/*
	 * Insert additional freeing code here, if needed
	 */

	return 1;
}
