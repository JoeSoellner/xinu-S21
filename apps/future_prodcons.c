#include <xinu.h>
#include <future.h>
#include <stddef.h>
#include <future_prodcons.h>

sid32 print_sem;

uint future_prod(future_t *fut, char *value, int state)
{
  int status;
  wait(print_sem);
  printf("Producing %d\n", *value);
  signal(print_sem);
  status = (int)future_set(fut, value);
  if (status < 1)
  {
    wait(print_sem);
    printf("future_set failed\n");
    signal(print_sem);
    return -1;
  }

  return OK;
}

uint future_cons(future_t *fut, int state)
{
  char *i = NULL;
  int status;
  status = (int)future_get(fut, i);
  if (status < 1)
  {
    wait(print_sem);
    printf("future_get failed\n");
    signal(print_sem);
    return -1;
  }
  wait(print_sem);
  printf("Consumed %d\n", *i);
  signal(print_sem);
  return OK;
}

/*
uint future_prod_save_state(future_t* fut, char* value, int state) {
  fut->state = state;
  uint res = future_prod(fut, value);
  return res;
}

uint future_cons_save_state(future_t* fut, int state) {
  fut->state = state;
  uint res = future_cons(fut);
  return res;
}
*/