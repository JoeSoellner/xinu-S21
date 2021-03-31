#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>
#include <stream_consumer.h>
#include <string.h>
#include <tscdf_input.h>

uint pcport;
int32 num_streams = -1;
int32 work_queue_depth = -1;
int32 time_window = -1;
int32 output_time = -1;

int stream_proc(int nargs, char *args[]) {

	// Parse arguments
	char usage[] = "run tscdf -s num_streams -w work_queue_depth -t time_window "
				   "-o output_time\n";

	// printf("nargs %d\n", nargs);
	if (nargs != 9) {
		printf("%s", usage);
		return (-1);
	} else {
		int i = nargs - 1;
		while (i > 0) {
			char *currArg = args[i - 1];
			// wtf c devs are crazy man
			char c = *(++currArg);

			switch (c) {
				case 's':
					num_streams = atoi(args[i]);
					break;
				case 'w':
					work_queue_depth = atoi(args[i]);
					break;
				case 't':
					time_window = atoi(args[i]);
					break;
				case 'o':
					output_time = atoi(args[i]);
					break;
				default:
					printf("%s", usage);
					return (-1);
			}
			i -= 2;
		}
	}

	ulong secs, msecs, time;
	secs = clktime;
	msecs = clkticks;

	if ((pcport = ptcreate(num_streams)) == SYSERR) {
		printf("ptcreate failed\n");
		return (-1);
	}

	// create num_streams number of streams
	struct stream streams[num_streams];

	// Create consumer processes and initialize streams
	for (int i = 0; i < num_streams; i++) {
		struct stream newStream = {
			.spaces = semcreate(work_queue_depth),
			.items = semcreate(0),
			.mutex = semcreate(1),
			.head = 0,
			.tail = 0,
			.queue = (de *)getmem(work_queue_depth * sizeof(de)),
		};

		streams[i] = newStream;
		resume(create((void *)stream_consumer, 4096, 20, "streamConsumer", 2, &streams[i], i));
	}

	// Parse input header file data and populate work queue
	for (int i = 0; i < n_input; i++) {
		char *a = (char *)stream_input[i];
		int streamID = atoi(a);
		while (*a++ != '\t')
			;
		int timestamp = atoi(a);
		while (*a++ != '\t')
			;
		int value = atoi(a);

		struct data_element currDataElement = {
			.time = timestamp,
			.value = value,
		};

    wait(streams[streamID].spaces);
    wait(streams[streamID].mutex);

    (streams[streamID].queue)[streams[streamID].head] = currDataElement;
    //struct data_element currItem = (de) (streams[streamID].queue)[streams[streamID].head];
    //printf("value %d received at time %d\n", currItem.value, currItem.time);
    
    //printf("value: %d\n", currItem.value);
    //printf("time: %d\n", currItem.time);
    streams[streamID].head = (streams[streamID].head + 1) % work_queue_depth;

    signal(streams[streamID].mutex);
    signal(streams[streamID].items);
	}

	for (int i = 0; i < num_streams; i++) {
		uint32 pm;
		pm = ptrecv(pcport);
		printf("process %d exited\n", pm);
	}

	ptdelete(pcport, 0);

	time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
	printf("time in ms: %u\n", time);

	return 0;
}