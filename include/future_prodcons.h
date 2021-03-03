#include <future.h>

uint future_prod(future_t* fut, char* value, int state);
uint future_cons(future_t* fut, int state);
uint future_prod_save_state(future_t* fut, char* value, int state);
uint future_cons_save_state(future_t* fut, int state);
void future_prodcons(int nargs, char *args[]);

extern char *val;
extern sid32 print_sem;