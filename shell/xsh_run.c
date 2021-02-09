#include <xinu.h>
#include <shprototypes.h>
 
shellcmd xsh_run(int nargs, char *args[]) {
    if ((nargs == 1) || (strncmp(args[1], "list", 4) == 0)) {
      printf("hello\n");
      printf("list\n");
	    printf("prodcons\n");
      return 1;
    }

    /* This will go past "run" and pass the function/process name and its
    * arguments.
    */
    args++;
    nargs--;

    if(strncmp(args[0], "hello", 5) == 0 && nargs == 2) {
      /* create a process with the function as an entry point. */
      resume (create((void *)xsh_hello, 4096, 20, "hello", 2, nargs, args));
	    return 1;
    }

	  if(strncmp(args[0], "prodcons", 8) == 0 && nargs == 2) {
      /* create a process with the function as an entry point. */
      resume (create((void *)xsh_prodcons, 4096, 20, "prodcons", 2, nargs, args));
	    return 1;
    }
	return 1;
}