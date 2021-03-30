#include <stream_consumer.h>

void stream_consumer(struct stream *givenStream, int32 streamId) {
	printf("stream_consumer id:%d (pid:%d)\n", streamId, getpid());

	int count = 0;
	while (1) {
		wait(givenStream->items);
		wait(givenStream->mutex);

		// get the item from the queue, remove it, and update head value
		// could 
		struct data_element currItem = (de) (givenStream->queue)[givenStream->head];
		//printf("value %d received at time %d, id:%d\n", currItem.value, currItem.time, streamId);
		// should be updating head but updating tail seems to make at least all threads run once
		givenStream->tail = (givenStream->tail + 1) % work_queue_depth;

		if (currItem.time == 0 && currItem.value == 0) {
			break;
		}

		printf("value %d received at time %d\n", currItem.value, currItem.time);

		signal(givenStream->mutex);
		signal(givenStream->spaces);
		count += 1;
	}

	printf("stream_consumer exiting\n");
}