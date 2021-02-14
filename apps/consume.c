#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>

void consumer(int count) {
	// reads the value of the global variable 'n'
	// 'count' times.
	// print consumed value e.g. consumed : 8

	for(int i = 0; i <= count; i++) {
		wait(can_read);
		printf("consumed : %d\n", n);
		signal(can_write);
	}
}

void consumer_bb(int id, int count) {
  // Iterate from 0 to count and for each iteration read the next available value from the global array `arr_q`
  // print consumer id (starting from 0) and read value as,
  // name : consumer_X, read : X

    	for(int i = 0; i < count; i++) {
		wait(readSem);
		wait(mutex);
		// shouldnt hardcode but i cant be bothered
		int currVal = arr_q[currRead];
		currRead = (currRead + 1) % 5;
		printf("name : consumer_%d, read : %d\n", id, currVal);
		//printf("name : consumer_%d, read : %d, currRead: %d\n", id, currVal, currRead);
		signal(mutex);
		signal(writeSem);
	}
}