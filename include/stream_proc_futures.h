#include <xinu.h>
#include <future.h>
#include <stream_proc.h>
#include <tscdf.h>

int stream_proc_futures(int nargs, char* args[]);
void stream_consumer_future(int32 id, future_t *f);