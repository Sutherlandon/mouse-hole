/*
 * Complete this file and run rpcgen -MN minifyjpeg.x
 */

struct jpeg {
	opaque image<>;
};

/*
union jpeg switch( int errno )
{
	case 0:
		jpeg small_image;
	default:
		void;
};
*/

program MINIFY_PROG {
	version MINIFY_VERS {
		jpeg MINIFY_JPEG( jpeg ) = 1;
	} = 1;
} = 0x20000001;
