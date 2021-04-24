#include <future_fib.h>
#include <future_prodcons.h>
#include <shprototypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stream_proc_futures.h>
#include <string.h>
#include <xinu.h>
#include <fs.h>

int fstest(int, char *);

void printPrograms() {
	printf("hello\n");
	printf("list\n");
	printf("prodcons_bb\n");
	printf("prodcons\n");
	printf("futest\n");
	printf("tscdf\n");
	printf("fstest\n");
}

shellcmd xsh_run(int nargs, char *args[]) {
	if ((nargs == 1) || (strncmp(args[1], "list", 4) == 0)) {
		printPrograms();
		return 1;
	}

	/* This will go past "run" and pass the function/process name and its
	 * arguments.
	 */
	args++;
	nargs--;

	if (strncmp(args[0], "hello", 5) == 0) {
		/* create a process with the function as an entry point. */
		resume(create((void *)xsh_hello, 4096, 20, "hello", 2, nargs, args));
		return 1;
	} else if (strncmp(args[0], "prodcons_bb", 11) == 0) {
		resume(create((void *)xsh_prodcons_bb, 4096, 20, "prodcons_bb", 2, nargs, args));
		return 1;
	} else if (strncmp(args[0], "prodcons", 8) == 0) {
		/* create a process with the function as an entry point. */
		resume(create((void *)xsh_prodcons, 4096, 20, "prodcons", 2, nargs, args));
		return 1;
	} else if (strncmp(args[0], "futest", 7) == 0) {
		/* create a process with the function as an entry point. */
		resume(create((void *)future_prodcons, 4096, 20, "futest", 2, nargs, args));
		return 1;
	} else if (strncmp(args[0], "tscdf_fq", 9) == 0) {
		/* create a process with the function as an entry point. */
		resume(create((void *)stream_proc_futures, 4096, 20, "tscdf_fq", 2, nargs, args));
		return 1;
	} else if (strncmp(args[0], "tscdf", 7) == 0) {
		/* create a process with the function as an entry point. */
		resume(create((void *)stream_proc, 4096, 20, "tscdf", 2, nargs, args));
		return 1;
	} else if (strncmp(args[0], "fstest", 6) == 0) {
		/* create a process with the function as an entry point. */
		resume(create((void *)fstest, 4096, 20, "fstest", 2, nargs, args));
		return 1;
	} else {
		printPrograms();
		return 1;
	}
}

char *val;

void future_prodcons(int nargs, char *args[]) {
	if (nargs < 2) {
		fprintf(stderr, "Syntax: run futest [-pc [g ...] [s VALUE ...]|-f "
						"NUMBER][--free]\n");
		return;
	}

	print_sem = semcreate(1);
	future_t *f_exclusive;
	if(strcmp(args[1], "-pcq") == 0) {
		f_exclusive = future_alloc(FUTURE_QUEUE, sizeof(int), atoi(args[2]));
	} else {
		f_exclusive = future_alloc(FUTURE_EXCLUSIVE, sizeof(int), 1);
	}

	// First, try to iterate through the arguments and make sure they are all
	// valid based on the requirements (you may assume the argument after "s"
	// there is always a number)
	for (int i = 1; i < nargs; i++) {
		// think i tried to do this in a more elegant way but was a pain
		// its ugly but it works
		if (strcmp(args[i], "-pc") == 0) {
			continue;
		} else if (strcmp(args[i], "-pcq") == 0) {
			continue;
		} else if (strcmp(args[i], "-f") == 0) {
			continue;
		} else if (strcmp(args[i], "--free") == 0) {
			continue;
		} else if (strcmp(args[i], "g") == 0) {
			continue;
		} else if (strcmp(args[i], "s") == 0) {
			continue;
		} else {
			int isNumber = 1;

			for (int j = 0; j < strlen(args[i]); j++) {
				char currChar = (char)args[i][j];
				if (currChar < '0' || currChar > '9') {
					isNumber = 0;
					break;
				}
			}

			if (isNumber == 1) {
				continue;
			} else {
				fprintf(stderr, "Syntax: run futest [-pc [g ...] [s VALUE "
								"...]|-f NUMBER][--free]\n");
				return;
			}
		}
	}

	// check flags
	if (strcmp(args[1], "-f") == 0) {
		if (nargs != 3) {
			fprintf(stderr, "Syntax: run futest [-pc [g ...] [s VALUE ...]|-f "
							"NUMBER][--free]\n");
			return;
		}

		resume(create(future_fib, 2048, 20, "fFibMain", 2, nargs, args));
	} else if (strcmp(args[1], "--free") == 0) {
		if (nargs != 2) {
			fprintf(stderr, "Syntax: run futest [-pc [g ...] [s VALUE ...]|-f "
							"NUMBER][--free]\n");
			return;
		}

		// just pass trash, not sure why it has params
		resume(create(future_free_test, 2048, 20, "futFreeTest", 2, 0, NULL));
	} else if (strcmp(args[1], "-pc") == 0 || strcmp(args[1], "-pcq") == 0) {
		if (nargs < 3) {
			if (strcmp(args[1], "-pc") == 0) {
				fprintf(stderr, "Syntax: run futest [-pc [g ...] [s VALUE ...]|-f "
					"NUMBER][--free]\n");
				return;
			} else if (strcmp(args[1], "-pcq") == 0) {
				fprintf(stderr, "run futest [-pc [g ...] [s VALUE ...]] | [-pcq LEN"
					"GTH [g ...] [s VALUE ...]] | [-f NUMBER] | [--free]\n");
				return;
			}
		}

		int num_args = nargs - 1;		// keeping number of args to create the array
		int i = 2;						// reseting the index
		val = (char *)getmem(num_args); // initializing the array to keep the "s" numbers

		// Iterate again through the arguments and create the following
		// processes based on the passed argument ("g" or "s VALUE")
		while (i < nargs) {
			if (strcmp(args[i], "g") == 0) {
				char id[10];
				sprintf(id, "fcons%d", i);
				resume(create(future_cons, 2048, 20, "fcons1", 1, f_exclusive));
			}
			if (strcmp(args[i], "s") == 0) {
				i++;
				uint8 number = atoi(args[i]);
				val[i] = number;
				resume(create(future_prod, 2048, 20, "fprod1", 2, f_exclusive, &val[i]));
				sleepms(5);
			}
			i++;
		}
	}
	sleepms(100);
	future_free(f_exclusive);
}