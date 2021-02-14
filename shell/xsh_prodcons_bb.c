#include <xinu.h>
#include <prodcons_bb.h>
#include <stdio.h>
#include <stdlib.h>       

// definition of array, semaphores and indices 
int arr_q[5];
sid32 writeSem;
sid32 readSem;
sid32 mutex;
int currWrite;
int currRead;

shellcmd xsh_prodcons_bb(int nargs, char *args[]) {
	if (nargs != 5) {
		fprintf(stderr, "Syntax: run prodcons_bb [# of producer processes] [# of consumer processes] [# of iterations the producer runs] [# of iterations the consumer runs]\n");
		return 1;
	}
	
	int numProducers = atoi(args[1]);
	int numConsumers = atoi(args[2]);
	int numProducerIterations = atoi(args[3]);
	int numConsumerIterations = atoi(args[4]);
	int count = numProducers * numProducerIterations;

	if(numProducers * numProducerIterations != numConsumers * numConsumerIterations) {
		fprintf(stderr, "Iteration Mismatch Error: the number of producer(s) iteration does not match the consumer(s) iteration\n");
		return 1;
	}

  	//create and initialize semaphores to necessary values
  	//initialize read and write indices for the queue
	for(int i = 0; i < 5; i ++) {
		arr_q[i] = 0;
	}
	writeSem = semcreate(5);
	readSem = semcreate(0);
	mutex = semcreate(1);
	currWrite = 0;
	currRead = 0;
  
  	//create producer and consumer processes and put them in ready queue
	for(int i = 0; i < numProducers; i++) {
		// all this for creating a producer_i string in each loop
		// i love C ! :)))
		/*
		char* producerID = calloc(sizeof(char), 10);
		char currIndex;
		sprintf(currIndex, "%d", i);
		producerID[0] = "producer_";
		producerID[10] = currIndex;
		*/
		resume(create(producer_bb, 1024, 20, "producer_bb", 2, i, count));
		//free(producerID);
	}

	for(int i = 0; i < numConsumers; i++) {
		resume(create(consumer_bb, 1024, 20, "consumer_bb", 2, i, count));
	}
	return 0;
}