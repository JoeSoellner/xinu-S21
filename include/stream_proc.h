#include <xinu.h>

typedef struct data_element {
	int32 time;
	int32 value;
} de;

struct stream {
	sid32 spaces;
	sid32 items;
	sid32 mutex;
	int32 head;
	int32 tail;
	struct data_element *queue;
};

// does this need to be a global?
extern int32 pcport;
extern int32 num_streams;
extern int32 work_queue_depth;
extern int32 time_window;
extern int32 output_time;

int stream_proc(int nargs, char *args[]);