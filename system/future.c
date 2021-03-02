#include <future.h>
#include <xinu.h>
#include <stddef.h>

future_t* future_alloc(future_mode_t mode, uint size, uint nelem)
{
  //should do some checking on params and return syserr if doesnt check out

  intmask mask;
  mask = disable();

  // init block of memory of size 'size' for data attribute of future
  char* tempData = getmem(size);

  struct future_t newFuture = {
    .data = tempData,
    .size = size,
    .state = FUTURE_EMPTY,
    .mode = mode
    // idk if i need to init pid
  };

  struct future_t *futurePtr = &newFuture;
  futurePtr = &newFuture;

  restore(mask);
  return futurePtr;
}

syscall future_free(future_t* f) {
  syscall freeMemResult = freemem(f->data, f->size);
  syscall killResult = kill(f->pid);

  if(freeMemResult == SYSERR || killResult == SYSERR) {
    return SYSERR;
  } else {
    return OK;
  }
}

syscall future_get(future_t* f,  char* out) {
  //printf("state: %d ", (int) f->state);
  if(f->state == FUTURE_EMPTY) {
    printf("hello\n");
    f->state = FUTURE_WAITING;
    return OK;
  } else if (f->state == FUTURE_WAITING) {
    printf("hello\n");
    return SYSERR;
  } else if (f->state == FUTURE_READY) {
    printf("hello\n");
    f->state = FUTURE_EMPTY;
    *out = *(f->data);
    return OK;
  }
  //printf("state: %d ", (int) f->state);
  return SYSERR;
}

syscall future_set(future_t* f, char* in) {
  //printf("state: %d ", (int) f->state);
  if(f->state == FUTURE_EMPTY) {
    printf("hello\n");
    f->state = FUTURE_READY;
    *(f->data) = *in;
    // idk if i need to set size to size of in's data
    return OK;
    // might need to do some extra stuff here, not sure
  } else if (f->state == FUTURE_WAITING) {
    printf("hello\n");
    f->state = FUTURE_EMPTY;
    return OK;
  } else if (f->state == FUTURE_READY) {
    printf("hello\n");
    return SYSERR;
  }
  //printf("state: %d ", (int) f->state);
  return SYSERR;
}