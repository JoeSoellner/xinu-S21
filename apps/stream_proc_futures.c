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

		future_t *currFuture = futures[streamID];
		future_set(currFuture, (char *) &currDataElement);
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
	kprintf("stream_consumer id:%d (pid:%d)\n", id, getpid());
	struct tscdf *tc = tscdf_init(time_window);

	int count = 0;
	while (1) {
		char *charPtr;

		future_get(f, charPtr);

		struct data_element *currItem = (de *) charPtr;

		if (currItem->time == 0 && currItem->value == 0) {
			break;
		}

		//kprintf("time: %d, value: %d\n", currItem->time, currItem->value);
		tscdf_update(tc, currItem->time, currItem->value);

		if(count == output_time - 1) {
			int32 *qarray = (int32 *)getmem((6 * sizeof(int32)));
			qarray = tscdf_quartiles(tc);

			if(qarray == NULL) {
				kprintf("tscdf_quartiles returned NULL\n");
				continue;
			}

			char output[64];
			sprintf(output, "s%d: %d %d %d %d %d", id, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
			kprintf("%s\n", output);
			freemem(qarray, (6 * sizeof(int32)));

			count = 0;
		} else {
      		count += 1;
    	}
	}

	// Free tscdf time window
	freemem((char *) tc, sizeof(struct tscdf));

	// Signal producer and exit
	kprintf("stream_consumer exiting\n");
	ptsend(pcport, getpid());

	return;
}