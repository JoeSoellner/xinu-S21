#include <future.h>
#include <xinu.h>
#include <stddef.h>

future_t* future_alloc(future_mode_t mode, uint size, uint nelem)
{
  intmask mask;
  mask = disable();

  // init block of memory of size 'size' for data attribute of future
  char* tempData = getmem(size);

  struct future_t newFuture = {
    .data = tempData,
    .size = size,
    .state = FUTURE_EMPTY,
    .mode = mode,
    .pid = 1
    // idk if i need to init pid
  };

  struct future_t *futurePtr = &newFuture;
  futurePtr = &newFuture;

  restore(mask);
  return futurePtr;
}

syscall future_free(future_t* f) {
  intmask mask;
  mask = disable();

  syscall freeMemResult = freemem(f->data, f->size);
  syscall killResult = kill(f->pid);

  restore(mask);
  if(freeMemResult == SYSERR || killResult == SYSERR) {
    return SYSERR;
  } else {
    return OK;
  }
}

syscall future_get(future_t* f,  char* out) {
  intmask mask;
  mask = disable();
  //printf("hello");
  //printf("state get : %d ", (int) f->state);
  if(f->state == FUTURE_EMPTY) {
    f->state = FUTURE_WAITING;
    f->pid = getpid();
    suspend(f->pid);
    memcpy(out, f->data, f->size);
    f->state = FUTURE_EMPTY;

    restore(mask);
    return OK;
  }
  
  if (f->state == FUTURE_WAITING) {
    restore(mask);
    return SYSERR;
  }
  
  if (f->state == FUTURE_READY) {
    memcpy(out, f->data, f->size);
    f->state = FUTURE_EMPTY;
    restore(mask);
    return OK;
  }

 /*
   if(f->state == FUTURE_EMPTY) {
    f->state = FUTURE_WAITING;
    return OK;
  } else if (f->state == FUTURE_WAITING) {
    return SYSERR;
  } else if (f->state == FUTURE_READY) {
    f->state = FUTURE_EMPTY;
    memcpy(out, f->data, f->size);
    return OK;
  }
  */
  //printf("get state: %d ", (int) f->state);
  return SYSERR;
}

syscall future_set(future_t* f, char* in) {
  intmask mask;
  mask = disable();
  //printf("state: %d ", (int) f->state);
  if(f->state == FUTURE_EMPTY) {
    //printf("in: %s, len: %d", in, strlen(in));
    //printf("hello");
    memcpy(f->data, in, strlen(in));
    f->size = strlen(in);
    f->state = FUTURE_READY;
    restore(mask);
    return OK;
  }
  
  if (f->state == FUTURE_WAITING) {
    resume(f->pid);
    f->state = FUTURE_EMPTY;
    restore(mask);
    return OK;
  }
  
  if (f->state == FUTURE_READY) {
    restore(mask);
    return SYSERR;
  }
    /*
   if(f->state == FUTURE_EMPTY) {
    f->state = FUTURE_READY;
    memcpy(f->data, in, strlen(in));
    // idk if i need to set size to size of in's data
    return OK;
    // might need to do some extra stuff here, not sure
  } else if (f->state == FUTURE_WAITING) {
    f->state = FUTURE_EMPTY;
    return OK;
  } else if (f->state == FUTURE_READY) {
    return SYSERR;
  }
  */
  //printf("set state: %d ", (int) f->state);
  return SYSERR;
}