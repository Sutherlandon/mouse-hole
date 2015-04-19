#include "minifyjpeg.h"
#include "magickminify.h"
#include "minifyjpeg.h"
/* Implement the needed server-side functions here */

bool_t minify_jpeg_1_svc( jpeg image, jpeg *result, struct svc_req *rqstp)
{
	bool_t retval;
	static jpeg small_image;

	// free any previous results
	xdr_free( xdr_jpeg, &small_image );

	// initialize the library
	magickminify_init();

	// minify the image and return the length parameter
	&small_image.image_val = (char *) magickminify( (void *) image, image_len, dest_len );

	// cleanup the library
	magickminify_cleanup();

	return retval;
}

int minify_prog_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free (xdr_result, result);

	/*
	 * Insert additional freeing code here, if needed
	 */

	return 1;
}
