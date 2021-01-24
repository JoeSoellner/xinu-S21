/* xsh_hello.c - xsh_hello */

//#include <stdio.h>
//#include <string.h>
#include <xinu.h>

/*------------------------------------------------------------------------
 * xsh_hello - print a greeting using the argument
 *------------------------------------------------------------------------
 */
shellcmd xsh_hello(int nargs, char *args[]) {

	/* Check argument count */
	if (nargs > 2) {
		fprintf(stderr, "%s: too many arguments\n", args[0]);
		return 1;
	} else if (nargs < 2) {
		fprintf(stderr, "%s: too few arguments\n", args[0]);
		return 1;
	} else {
		printf("Hello %s, Welcome to the world of Xinu!!\n", args[1]);
		return 0;
	}
	return 1;
}