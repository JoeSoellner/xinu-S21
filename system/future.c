#include <future.h>
#include <stddef.h>
#include <stream_proc.h>

future_t* future_alloc(future_mode_t mode, uint size, uint nelem)
{
  intmask mask;
  mask = disable();

  struct future_t *futurePtr = (struct future_t*) getmem(sizeof(future_t));
  futurePtr->size = size;
  futurePtr->state = FUTURE_EMPTY;
  futurePtr->mode = mode;
  futurePtr->pid = -1;
  // init block of memory of size 'size' for data attribute of future
  // data attribute is used as a queue in queue mode so is of size * numOfElements
  futurePtr->data = mode == FUTURE_QUEUE ? getmem(size * nelem) : getmem(size);

  if(mode == FUTURE_SHARED) {
    futurePtr->get_queue = newqueue();
  } else if (mode == FUTURE_QUEUE) {
    futurePtr->get_queue = newqueue();
    futurePtr->set_queue = newqueue();
    futurePtr->max_elems = nelem;
    futurePtr->count = 0;
    futurePtr->head = 0;
    futurePtr->tail = 0;
  }

  restore(mask);
  return futurePtr;
}

syscall future_free(future_t* f) {
  intmask mask;
  mask = disable();
  syscall killResult = OK;

  // have to free more memory in queue mode as data a queue of elements
  // instead of a single element
  syscall freeMemResult = f->mode == FUTURE_QUEUE ? freemem(f->data, f->size * f->max_elems) : freemem(f->data, f->size);

  if(f->pid != -1) {
    killResult = kill(f->pid);
  }

  if(f->mode == FUTURE_SHARED) {
    while(nonempty(f->get_queue)) {
      kill(dequeue(f->get_queue));
    }
  } else if (f->mode == FUTURE_QUEUE) {
    while(nonempty(f->get_queue)) {
      kill(dequeue(f->get_queue));
    }

    while(nonempty(f->set_queue)) {
      kill(dequeue(f->set_queue));
    }
  }

  syscall freeFuturePtrResult = freemem((char *) f, sizeof(future_t));

  restore(mask);
  if(freeMemResult == SYSERR || killResult == SYSERR || freeFuturePtrResult == SYSERR) {
    return SYSERR;
  } else {
    return OK;
  }
}

syscall future_get(future_t* f, char* out) {
  intmask mask;
  mask = disable();

  if(f->mode == FUTURE_EXCLUSIVE) {
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
  } else if(f->mode == FUTURE_SHARED) {
    if(f->state == FUTURE_EMPTY) {
      f->state = FUTURE_WAITING;
      enqueue(f->get_queue, getpid());
      suspend(getpid());

      memcpy(out, f->data, f->size);

      restore(mask);
      return OK;
    }
    
    if (f->state == FUTURE_WAITING) {
      enqueue(f->get_queue, getpid());
      suspend(getpid());

      memcpy(out, f->data, f->size);

      restore(mask);
      return OK;
    }
    
    if (f->state == FUTURE_READY) {
      memcpy(out, f->data, f->size);

      restore(mask);
      return OK;
    }

    return SYSERR;
  } else if (f->mode == FUTURE_QUEUE) {
    // no items to get, so go in waiting queue
    // when future wakes up get an element from the head of the queue
    if(f->count == 0) {
      enqueue(getpid(), f->get_queue);
      suspend(getpid());
    }

    char* headelemptr = f->data + (f->head * f->size);
    memcpy(out, headelemptr, f->size);
    f->head = (f->head + 1) % f->max_elems;
    f->count -= 1;

    // now that there is room, resume a setter if one is waiting
    if(nonempty(f->set_queue)) {
      resume(dequeue(f->set_queue));
    }

    restore(mask);
    return OK;
  }
  return SYSERR;
}

syscall future_set(future_t* f, char* in) {
  intmask mask;
  mask = disable();

  if(f->mode == FUTURE_EXCLUSIVE) {
    if(f->state == FUTURE_EMPTY) {
      memcpy(f->data, in, strlen(in));
      f->size = strlen(in);
      f->state = FUTURE_READY;
      restore(mask);
      return OK;
    }
    
    if (f->state == FUTURE_WAITING) {
      memcpy(f->data, in, strlen(in));
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
  } else if(f->mode == FUTURE_SHARED) {
    if(f->state == FUTURE_EMPTY) {
      memcpy(f->data, in, strlen(in));
      f->size = strlen(in);
      f->state = FUTURE_READY;

      restore(mask);
      return OK;
    }
    
    if (f->state == FUTURE_WAITING) {
      memcpy(f->data, in, strlen(in));
      f->size = strlen(in);
      f->state = FUTURE_READY;

      // empty all the waiting threads and resume them
      while(nonempty(f->get_queue)) {
        resume(dequeue(f->get_queue));
      }

      restore(mask);
      return OK;
    }
    
    if (f->state == FUTURE_READY) {
      restore(mask);
      return SYSERR;
    }
    return SYSERR;
  } else if (f->mode == FUTURE_QUEUE) {
    // queue is full, so wait until there is space
    // when future wakes up put the element in the tail of the queue
    if(f->count == f->max_elems) {
      enqueue(getpid(), f->set_queue);
      suspend(getpid());
    }

    char* tailelemptr = f->data + (f->tail * f->size);
    memcpy(tailelemptr, in, sizeof(de));
    f->tail = (f->tail + 1) % f->max_elems;
    f->count += 1;

    // now that there is a item to get, resume a getter if one is waiting
    if(nonempty(f->get_queue)) {
      resume(dequeue(f->get_queue));
    }

    restore(mask);
    return OK;
  }
  return SYSERR;
}