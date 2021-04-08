#include <stream_proc_futures.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int stream_proc_futures(int nargs, char* args[]) {
  	char usage[] = "Usage: run tscdf_fq -s num_streams -w work_queue_depth -t time_window -o output_time\n";

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

	// Create array to hold `num_streams` futures
	// struct stream streams[num_streams];
	future_t **futures = (struct future_t**) getmem(sizeof(future_t) * num_streams);

	for (int i = 0; i < num_streams; i++) {
		future_t *newFuture = future_alloc(FUTURE_QUEUE, sizeof(de), work_queue_depth);

		futures[i] = newFuture;
		resume(create((void *)stream_consumer_future, 4096, 20, "streamConsumerFuture", 2, i, futures[i]));
	}

	// Parse input header file data and set future values
	struct data_element currDataElement;
	for (int i = 0; i < n_input; i++) {
		char *a = (char *)stream_input[i];
		int streamID = atoi(a);
		while (*a++ != '\t')
			;
		int timestamp = atoi(a);
		while (*a++ != '\t')
			;
		int value = atoi(a);

		currDataElement.time = timestamp;
		currDataElement.value = value;

		future_set(futures[streamID], (char *) &currDataElement);
	}

	// Wait for all consumers to exit
	for (int i = 0; i < num_streams; i++) {
		uint32 pm;
		pm = ptrecv(pcport);
		printf("process %d exited\n", pm);
	}

	// Free all futures
	for (int i = 0; i < num_streams; i++) {
		future_free(futures[i]);
	}

	// Timing: Tock+Report
	ptdelete(pcport, 0);

	time = (((clktime * 1000) + clkticks) - ((secs * 1000) + msecs));
	printf("time in ms: %u\n", time);

	return 0;
}

void stream_consumer_future(int32 id, future_t *f) {
	kprintf("stream_consumer_future id:%d (pid:%d)\n", id, getpid());

	struct tscdf *tc = tscdf_init(time_window);
	struct data_element data_element;

	int32 *qarray;
	char output[64];
	int count = 0;
	
	while (1) {
		future_get(f, (char  *) &data_element);

		if (data_element.time == 0 && data_element.value == 0) {
			break;
		}

		tscdf_update(tc, data_element.time, data_element.value);

		if(count == output_time - 1) {
			qarray = tscdf_quartiles(tc);

			if(qarray == NULL) {
				kprintf("tscdf_quartiles returned NULL\n");
				continue;
			}

			sprintf(output, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
			kprintf("%s\n", output);
			freemem(qarray, (6 * sizeof(int32)));

			count = 0;
		} else {
      		count += 1;
    	}
	}

	tscdf_free(tc);

	// Signal producer and exit
	kprintf("stream_consumer_future exiting\n");
	ptsend(pcport, getpid());

	return;
}