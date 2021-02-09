#include <xinu.h>
#include <prodcons.h>
#include <stdio.h>
#include <stdlib.h>

/*Now global variable n will be on Heap so it is accessible all the processes i.e. consume and produce*/
int n;
sid32 can_write;
sid32 can_read;
sid32 mutex;

shellcmd xsh_prodcons(int nargs, char *args[])
{
	if (nargs > 2) {
		fprintf(stderr, "%s: too many arguments\n", args[0]);
		return 1;
	}
	
	// if optional arg is given then set count to it, otherwise set to 2000
	int count;
	if (nargs == 2) {
		int arg1ToInt = atoi(args[1]);
		count = arg1ToInt;
	} else {
		count = 200;
	}
	
	// init semaphores
	can_write = semcreate(1);
	can_read = semcreate(0);

	//create the process producer and consumer and put them in ready queue.
	//Look at the definations of function create and resume in the system folder for reference.
	resume(create(producer, 1024, 20, "producer", 1, count));
	resume(create(consumer, 1024, 20, "consumer", 1, count));
	return (0);
}