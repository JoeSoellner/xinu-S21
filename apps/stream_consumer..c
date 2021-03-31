#include <stream_consumer.h>

void stream_consumer(struct stream *givenStream, int32 streamId) {
	kprintf("stream_consumer id:%d (pid:%d)\n", streamId, getpid());

	int count = 0;
	while (1) {
		wait(givenStream->items);
		wait(givenStream->mutex);

		// get the item from the queue, remove it, and update tail value
		// could 
		struct data_element currItem = (de) (givenStream->queue)[givenStream->tail];
		//printf("value %d received at time %d, id:%d\n", currItem.value, currItem.time, streamId);
		// should be updating tail but updating tail seems to make at least all threads run once
		givenStream->tail = (givenStream->tail + 1) % work_queue_depth;

		if (currItem.time == 0 && currItem.value == 0) {
			break;
		}

		kprintf("value %d received at time %d\n", currItem.value, currItem.time);

		signal(givenStream->mutex);
		signal(givenStream->spaces);
		count += 1;
	}

	kprintf("stream_consumer exiting\n");
}
