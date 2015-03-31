#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "gfserver.h"
#include "shm_channel.h"
#include "simplecache.h"

#define USAGE                                                                   \
	"usage:\n"                                                                      \
"  webproxy [options]\n"                                                        \
"options:\n"                                                                    \
"  -p [listen_port]    Listen port (Default: 8888)\n"                           \
"  -t [thread_count]   Num worker threads (Default: 1, Range: 1-1000)\n"        \
"  -s [server]         The server to connect to (Default: Udacity S3 instance)" \
"  -h                  Show this help message\n"                                \
"special options:\n"                                                            \
"  -d [drop_factor]    Drop connects if f*t pending requests (Default: 5).\n"

#define CACHE_OK 100

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
	{"port",          required_argument,      NULL,           'p'},
	{"thread-count",  required_argument,      NULL,           't'},
	{"server",        required_argument,      NULL,           's'},
	{"num_segments",  required_argument,      NULL,           'n'},
	{"segment_size",  required_argument,      NULL,           'z'},
	{"help",          no_argument,            NULL,           'h'},
	{NULL,            0,                      NULL,             0}
};

extern ssize_t handle_with_cache(gfcontext_t *ctx, char *path, void* arg);

static gfserver_t gfs;
int num_segments = 10;

static void _sig_handler(int signo){
	//int i;
	if (signo == SIGINT || signo == SIGTERM){
	//	for( i=0; i < num_segments; i++ )
	//		shmdt( (void *) segments[i]->seg_id );
		gfserver_stop(&gfs);
		exit(signo);
	}
}

// print errors and kill the program
void error( const char* msg ){
	fprintf( stderr, msg );
	exit(EXIT_FAILURE);
}


/* Main ========================================================= */
int main(int argc, char **argv)
{
	int i, option_char = 0;
	int segment_size = 1024;
	unsigned short port = 8888;
	unsigned short nworkerthreads = 1;
	char *server = "s3.amazonaws.com/content.udacity-data.com";

	if (signal(SIGINT, _sig_handler) == SIG_ERR){
		fprintf(stderr,"Can't catch SIGINT...exiting.\n");
		exit(EXIT_FAILURE);
	}

	if (signal(SIGTERM, _sig_handler) == SIG_ERR){
		fprintf(stderr,"Can't catch SIGTERM...exiting.\n");
		exit(EXIT_FAILURE);
	}

	// Parse and set command line arguments
	while ((option_char = getopt_long(argc, argv, "p:t:s:n:z:h", gLongOptions, NULL)) != -1) {
		switch (option_char) {
			case 'p': // listen-port
				port = atoi(optarg);
				break;
			case 't': // thread-count
				nworkerthreads = atoi(optarg);
				break;
			case 's': // file-path
				server = optarg;
				break;
			case 'n': // number of segments
				num_segments = atoi(optarg);
				break;
			case 'z': // size of each segment
				segment_size = atoi(optarg);
				break;
			case 'h': // help
				fprintf(stdout, "%s", USAGE);
				exit(0);
				break;
			default:
				fprintf(stderr, "%s", USAGE);
				exit(1);
		}
	}


	/* SHM initialization...*/
	// create the segments
	printf( "creating segments...\n" );
	int seg_id;
	shm_segment_t segments[num_segments];
	shm_segment_t seg_ptr = malloc( sizeof( shm_data_struct ));
	for( i = 0; i < num_segments; i++ )
	{
		seg_id = shmget( ftok( argv[0], i ), segment_size, IPC_CREAT/*|IPC_EXCL*/ );
		seg_ptr = (shm_segment_t) shmat( seg_id, (void *) 0, 0 );
		seg_ptr->seg_id = seg_id;
		seg_ptr->not_used = 1;
		seg_ptr->sem = 1;
		segments[i] = seg_ptr;
	}


	/*Initializing server*/
	gfserver_init(&gfs, nworkerthreads);

	/*Setting options*/
	gfserver_setopt(&gfs, GFS_PORT, port);
	gfserver_setopt(&gfs, GFS_MAXNPENDING, 10);
	gfserver_setopt(&gfs, GFS_WORKER_FUNC, handle_with_cache);
	for(i = 0; i < nworkerthreads; i++)
		gfserver_setopt( &gfs, GFS_WORKER_ARG, i, server );

	/*Loops forever*/
	gfserver_serve(&gfs);
}
