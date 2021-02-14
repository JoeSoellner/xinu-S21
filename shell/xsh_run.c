#include <xinu.h>
#include <shprototypes.h>

void printPrograms() {
  printf("hello\n");
  printf("list\n");
  printf("prodcons_bb\n");
  printf("prodcons\n");
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

    if(strncmp(args[0], "hello", 5) == 0) {
      /* create a process with the function as an entry point. */
      resume (create((void *)xsh_hello, 4096, 20, "hello", 2, nargs, args));
	    return 1;
    } else if(strncmp(args[0], "prodcons_bb", 11) == 0) {
      resume (create((void *)xsh_prodcons_bb, 4096, 20, "prodcons_bb", 2, nargs, args));
	    return 1;
    } else if(strncmp(args[0], "prodcons", 8) == 0) {
      /* create a process with the function as an entry point. */
      resume (create((void *)xsh_prodcons, 4096, 20, "prodcons", 2, nargs, args));
	    return 1;
    } else {
      printPrograms();
      return 1;
    }
}