#include <xinu.h>
#include <prodcons.h>
#include <prodcons_bb.h>

void producer(int count) {
	// Iterates from 0 to count, setting
    // the value of the global variable 'n'
    // each time.
    // print produced value e.g. produced : 8

	for(int i = 0; i <= count; i++) {
		wait(can_write);
		n = i;
		printf("produced : %d\n", i);
		signal(can_read);
	}
}

void producer_bb(int id, int count) {
  // Iterate from 0 to count and for each iteration add iteration value to the global array `arr_q`, 
  // print producer id (starting from 0) and written value as,
  // name : producer_X, write : X

  	for(int i = 0; i < count; i++) {
		wait(writeSem);
		wait(mutex);
		arr_q[currWrite] = i;
		// shouldnt hardcode but i cant be bothered
		currWrite = (currWrite + 1) % 5;
		printf("name : producer_%d, write : %d\n", id, i);
		//printf("name : producer_%d, write : %d, currWrite: %d \n", id, i, currWrite);
		signal(mutex);
		signal(readSem);
	}
}
