#include "lwp.h"
#include <stdint.h>

/* TODO: This was the scheduler example, may need to be tweaked */
static struct scheduler rr_publish = {NULL, NULL, rr_admit, rr_remove, rr_next}
scheduler RoundRobin = &rr_publish;
static unsigned int process_count = 0;
/* List will be linked in a circle, but need a point of reference */
static thread thread_head = NULL;
static rfile main_rfile = NULL;
static int started = 0; /* Set if lwp_start()'ed, cleared if lwp_stop()'d */

/* Create a new LWP */
tid_t lwp_create(lwpfun function, void *argument, size_t stack_size) {
   unsigned long *stack;
   rfile state_old, state_new;
   thread new_lwp;
   new_lwp->stacksize = stack_size;

   /* Assign process id based on process count */
   new_lwp->tid = (tid_t)++process_count;

   /* Allocate space for the stack 
    * TODO: Might have to change stack size to be in words, not bytes */
   stack = (unsigned long*)malloc(stack_size);
   
   /* Stack in context refers to the base of the stack so it can be freed */
   new_lwp->stack = stack;

   /* Move stack_pointer to the "top" of the stack */
   stack += stack_size;

   /* Save old context to get old base pointer, etc...*/
   load_context(state_old);
   
   /* From notes? rdi gets the argument in create */
   state_new.rdi = argument;
   state_new.rsp = stack;
   
   /* TODO: Figure out what to do with the "function" argument */

   /* Build up stack to look as though it were just called */
   /* Return address (lwp_exit) */
   --(unsigned long*)state_new.rsp = &lwp_exit;
   /* Push old base pointer on the stack */
   --(unsigned long*)state_new.rsp  = state_new.rbp
   /* Set base pointer to location of old base pointer */
   state_new.rbp = state_new.rsp;

   /* Initialize floating point unit */
   state_new.fxsave=FPU_INIT;

  /* Save state in new_lwp */ 
   new_lwp->state = state_new;

   /* TODO: Figure out what to do with schedulers in new_lwp */

   /* Default: Make the lwp loop with itself so the scheduler->next() will just
    * run the same thread again if there is only one lwp in the list */
   new_lwp->next = new_lwp;
   new_lwp->prev = new_lwp;

   /* Connect linked list of threads */
   if(!thread_head) {
      thread_head = new_lwp;      
   }
   else if(!thread_head->prev) {
      /* Create a loop out of two commands */
      new_lwp->next = thread_head;
      new_lwp->prev = thread_head;
      thread_head->next = new_lwp;
      thread_head->prev = new_lwp;
   }
   else {
      /* Put the new process behind thread_head */
     new_lwp->next = thread_head;
     new_lwp->prev = thread_head->prev;
     (thread_head->prev)->next = new_lwp;
     thread_head->prev = new_lwp;
   }
   return new_lwp->tid;
}


/* Return thread ID of the calling LWP */
tid_t lwp_gettid(void) {
    //TODO still not sure how a thread knows who it is.
}


/* Terminates the calling LWP. */
void lwp_exit(void) {
    thread nxt;

    //TODO Terminates the thread
    //TODO Frees the resources

    /* Get the next LWP to run. */
    nxt = RoundRobin->next();
    if(nxt == NULL) {
        /* No more LWP so restore main thread's rfile. */
        started = 0; /* We are effectively stopping the LWP process. */
        load_context(main_rfile);
    }
    else {
        /* Switch to nxt's rfile. */
        load_context(nxt->state);
    }
    return
}


/* Yield the CPU to another LWP. This consists of the current LWP swapping 
 * rfiles with the next LWP. If there is none, return to main rfile. */
void lwp_yield(void) {
    thread nxt,cur;
    
    /* Get the next LWP to run. */
    nxt = RoundRobin->next();
    if(nxt == NULL) {
        /* No more LWP so restore main thread's rfile. */
        started = 0; /* We are effectively stopping the LWP process. */
        load_context(main_rfile);
    }
    else {
        /* Switch to nxt's rfile. */
        cur = (RoundRobin->next())->prev; //TODO  Grab context of current thread. This method is iffy.
        swap_rfiles(cur->state,nxt->state);
    }
    return;
}


/* Start the LWP system. This consists of saving the main file to a global,
 * switch rfiles to the next LWP if it exists, else return. */
void lwp_start(void) {
    
    // ** This seems almost done. ** //
    
    thread nxt;

    if(process_count == 0)
        return;
    if(started == 1) {
        fprintf(stderr, "ERROR: Tried to start an already started LWP.\n");
        return;
    }

    /* Store the main thread's rfile in main_rfile. */
    load_context(main_rfile);

    /* Get the next LWP to run. */
    nxt = RoundRobin->next(); 
    if(nxt == NULL) {
        /* No next thread to run, return to main. */
        return;
    }
    else {
        started = 1;
        /* Switch to nxt's rfile. */
        swap_rfiles(main_rfile,nxt->state);
    }
    return;
}


/* Stop the LWP system. Returns us to where lwp_start() was initially called. 
 * This consists of restoring the main thread's rfile and [**setting the stack
 * pointer**] */
void lwp_stop(void) {
    thread nxt, cur;
    
    if(started == 0) {
        fprintf(stderr, "ERROR: Tried to stop a non-started LWP.\n");
        return;
    }
    started = 0;

    //TODO Restore orig stack pointer?

    nxt = RoundRobin->next();
    if(nxt == NULL) { 
        //TODO we are fucked. Cant get current thread info w/o it.
    }
    else {
        cur = (RoundRobin->next())->prev; //TODO Grab context of current thread. This method is iffy.
        /* Switch to main's rfile. */
        swap_rfiles(cur->state,main_rfile);
    }
    return;
}




/* Install a new scheduling function */
void lwp_set_scheduler(scheduler fun) {

    return;
}


/* Find out what the current scheduler is */
scheduler lwp_get_scheduler(void) {
   return rr_publish;   
}


/* Map a thread ID to a context */
thread tid2thread(tid_t tid) {
   thread temp_thread = thread_head;
   
   /* Special case for 0 or 1 thread(s) in the list */
   if(!thread_head) 
      return NULL;
   else if(thread_head->tid == tid) 
      return thread_head;
   temp_thread = temp_thread->next;

   /* Loop until we get back to thread_head */
   while(temp_thread && temp_thread != thread_head) {
      if(temp_thread->tid == tid)
         return temp_thread;
      temp_thread = temp_thread->next;
   }
   return NULL;
}
