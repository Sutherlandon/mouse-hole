//In case you want to implement the shared memory IPC as a library...

// data structure for the segment
typedef struct {
	int seg_id;
	int not_used;
	char *data;

	int sem;
} shm_data_struct, *shm_segment_t;

extern shm_segment_t segments[];
extern int num_segments;

