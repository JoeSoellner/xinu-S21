#include <xinu.h>
#include <prodcons.h>

void producer(int count) {
	// Iterates from 0 to count, setting
    // the value of the global variable 'n'
    // each time.
    //print produced value e.g. produced : 8
	printf("producer: before wait, can_write: %d, can_read: %d\n", semcount(can_write), semcount(can_read));
	wait(can_write);
	//wait(mutex);
	printf("producer: after wait, can_write: %d, can_read: %d\n", semcount(can_write), semcount(can_read));
	for(int i = 0; i < count; i++) {
		n = i;
		printf("produced: %d\n", i);
	}
	//signal(mutex);
	signal(can_read);
}



