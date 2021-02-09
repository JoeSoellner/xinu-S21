#include <xinu.h>
#include <prodcons.h>

void consumer(int count) {
	// reads the value of the global variable 'n'
	// 'count' times.
	// print consumed value e.g. consumed : 8
	//printf("consumer: before wait, can_write: %d, can_read: %d\n", semcount(can_write), semcount(can_read));
	wait(can_read);
	//wait(mutex);
	//printf("consumer: after wait, can_write: %d, can_read: %d\n", semcount(can_write), semcount(can_read));
	for(int i = 0; i < count; i++) {
		printf("consumed: %d\n", n);
	}
	//signal(mutex);
	signal(can_write);
}
