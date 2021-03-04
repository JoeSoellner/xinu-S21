#include <future.h>
#include <xinu.h>
#include <stddef.h>

future_t* future_alloc(future_mode_t mode, uint size, uint nelem)
{
  intmask mask;
  mask = disable();

  // init block of memory of size 'size' for data attribute of future
  char* tempData = getmem(size);

  struct future_t *futurePtr = getmem(sizeof(future_t));
  futurePtr->data = tempData;
  futurePtr->size = size;
  futurePtr->state = FUTURE_EMPTY;
  futurePtr->mode = mode;
  futurePtr->pid = -1;

  restore(mask);
  return futurePtr;
}

syscall future_free(future_t* f) {
  intmask mask;
  mask = disable();
  syscall killResult = OK;

  syscall freeMemResult = freemem(f->data, f->size);
  if(f->pid != -1) {
    killResult = kill(f->pid);
  }
  syscall freeFuturePtrResult = freemem(f, sizeof(future_t));

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
  return SYSERR;
}

syscall future_set(future_t* f, char* in) {
  intmask mask;
  mask = disable();

  if(f->state == FUTURE_EMPTY) {
    memcpy(f->data, in, strlen(in));
    f->size = strlen(in);
    f->state = FUTURE_READY;
    restore(mask);
    return OK;
  }
  
  if (f->state == FUTURE_WAITING) {
    resume(f->pid);
    f->pid = -1;
    f->state = FUTURE_EMPTY;
    restore(mask);
    return OK;
  }
  
  if (f->state == FUTURE_READY) {
    restore(mask);
    return SYSERR;
  }
  return SYSERR;
}