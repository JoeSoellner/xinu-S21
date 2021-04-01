#include <stream_consumer.h>
#include <tscdf.h>

void stream_consumer(struct stream *givenStream, int32 streamId) {
	kprintf("stream_consumer id:%d (pid:%d)\n", streamId, getpid());

	struct tscdf *tc = tscdf_init(time_window);

	int count = 0;
	while (1) {
		wait(givenStream->items);
		wait(givenStream->mutex);

		// get the item from the queue, remove it, and update tail value
		struct data_element currItem = (de) (givenStream->queue)[givenStream->tail];
		//printf("value %d received at time %d, id:%d\n", currItem.value, currItem.time, streamId);
		// should be updating tail but updating tail seems to make at least all threads run once
		givenStream->tail = (givenStream->tail + 1) % work_queue_depth;

		if (currItem.time == 0 && currItem.value == 0) {
			break;
		}

		tscdf_update(tc, currItem.time, currItem.value);

		if(count == output_time - 1) {
			// might have to define qarray in different namespace
			int32 *qarray = (int32 *)getmem((6 * sizeof(int32)));
			qarray = tscdf_quartiles(tc);

			if(qarray == NULL) {
				kprintf("tscdf_quartiles returned NULL\n");
				continue;
			}

			char output[64];
			// id = streamID?
			sprintf(output, "s%d: %d %d %d %d %d", streamId, qarray[0], qarray[1], qarray[2], qarray[3], qarray[4]);
			kprintf("%s\n", output);
			freemem((char *)qarray, (6 * sizeof(int32)));

			count = 0;
		} else {
      count += 1;
    }


		signal(givenStream->mutex);
		signal(givenStream->spaces);
	}

	kprintf("stream_consumer exiting\n");
	ptsend(pcport, getpid());
}
