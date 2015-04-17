#include "minifyjpeg_xdr.c"
#include "minifyjpeg_clnt.c"

void* minify_via_rpc(char* server, void* src_val, size_t src_len, size_t *dst_len){
  /*
    * Use RPC to obtain a minified version of the jpeg image
    * stored in the array src_val and having src_len bytes.
    */

}