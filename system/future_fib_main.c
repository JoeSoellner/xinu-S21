#include <future.h>
#include <future_fib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

future_t** fibfut;
int zero = 0;
int one = 1;
int two = 2;

int future_fib(int nargs, char *args[]){
  	int fib = -1, i;
    fib = atoi(args[2]);

    if (fib > -1) {
		int final_fib;
		int future_flags = FUTURE_SHARED;

		// create the array of future pointers
		if ((fibfut = (future_t **)getmem(sizeof(future_t *) * (fib + 1)))
			== (future_t **) SYSERR) {
			printf("getmem failed\n");
			return(SYSERR);
		}

		// get futures for the future array
		for (i=0; i <= fib; i++) {
			if((fibfut[i] = future_alloc(future_flags, sizeof(int), 1)) == (future_t *) SYSERR) {
			printf("future_alloc failed\n");
			return(SYSERR);
			}
		}

		// spawn fib threads and get final value
		for (i=0; i <= fib; i++) {
			resume(create(ffib, 2048, 20, "futureFib", 1, i));
		}

		future_get(fibfut[fib], (char*) &final_fib);

		for (i=0; i <= fib; i++) {
			future_free(fibfut[i]);
		}

		freemem((char *)fibfut, sizeof(future_t *) * (fib + 1));
		printf("Nth Fibonacci value for N=%d is %d\n", fib, final_fib);
		return(OK);
    }
	return SYSERR;
}

int future_free_test(int nargs, char *args[]) {
    future_t *f_exclusive;
    f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1);
    printf("future exclsive created\n");
    future_free(f_exclusive);
    printf("future exclsive freed\n");

    future_t *f_shared;
    f_shared = future_alloc(FUTURE_SHARED, sizeof(int), 1);
    printf("future shared created\n");
    future_free(f_shared);
    printf("future shared freed\n");

    return OK;
}