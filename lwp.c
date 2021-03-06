/* This is lwp.c.  Written by Ryan Kruger and Evin Killian. */

#include "lwp.h"
#include "rr.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* A semi-arbitrary ammount of space to fully exit a LWP. */
#define SAFESIZE 6400

static struct scheduler rr_publish = {NULL, NULL, rr_admit, rr_remove, rr_next};
static scheduler RoundRobin = &rr_publish;
static unsigned int process_count = 0;
/* List will be linked in a circle, but need a point of reference */
static thread thread_head = NULL;
static rfile main_rfile;
static int started = 0; /* Set if lwp_start()'ed, cleared if lwp_stop()'d */
static thread running_th = NULL;
static unsigned long safespace[SAFESIZE]; /* Buffer to hold reallyExit() call */


/* Prototypes. */
static void add_thread(thread new_lwp);
static void reallyExit(void);
static void remove_thread(thread lwp);


/* Create a new LWP */
tid_t lwp_create(lwpfun function, void *argument, size_t stack_size) {
   unsigned long *stack;
   rfile state_new;
   
   if(stack_size < 3) {
      fprintf(stderr, "Stack size needs to be at least 3 words");
      exit(1);
   }
   thread new_lwp = (thread)malloc(sizeof(context));
   if(!new_lwp) {
      fprintf(stderr, "Out of memory");
      exit(1);
   }
   new_lwp->stacksize = stack_size;
   
   /* Assign process id based on process count */
   new_lwp->tid = (tid_t)++process_count;

   /* Allocate space for the stack */
   stack = (unsigned long*)malloc(stack_size*sizeof(unsigned long));
   if(!stack) {
      fprintf(stderr, "Out of memory");
      exit(1);
   } 
   /* Stack in context refers to the base of the stack so it can be freed */
   new_lwp->stack = stack;

   /* rdi gets the argument in create */
   state_new.rdi = (unsigned long)argument;
   /* Move stack pointer to the top of the stack */
   state_new.rsp = (unsigned long)(stack += stack_size);
   
   /* Build up stack to look as though it were just called */
   /* Return address (lwp_exit) */
   state_new.rsp = state_new.rsp - sizeof(unsigned long);
   *((unsigned long*)(state_new.rsp)) = (unsigned long)&lwp_exit;
   /* From class, push function onto stack to be popped with return */
   state_new.rsp = state_new.rsp - sizeof(unsigned long);
   *((unsigned long*)(state_new.rsp)) = (unsigned long)function;
   /* Push old base pointer on the stack */
   state_new.rsp = state_new.rsp - sizeof(unsigned long);
   *((unsigned long*)(state_new.rsp)) = state_new.rbp;
   /* Set base pointer to location of old base pointer */
   state_new.rbp = state_new.rsp;

   /* Initialize floating point unit */
   state_new.fxsave=FPU_INIT;

   /* Save state in new_lwp */ 
   new_lwp->state = state_new;

   /* Add new_lwp to scheduler list */
   RoundRobin->admit(new_lwp);

   /* Default: Make the lwp loop with itself so the scheduler->next() will just
    * run the same thread again if there is only one lwp in the list */
   new_lwp->next = new_lwp;
   new_lwp->prev = new_lwp;

   add_thread(new_lwp);
   return new_lwp->tid;
}


/* Return thread ID of the calling LWP */
tid_t lwp_gettid(void) {
    return running_th->tid;
}


/* Terminates the calling LWP. */
void lwp_exit(void) {

    /* Move stack pointer to our buffer space */
    SetSP(safespace+SAFESIZE);
    reallyExit();

    return;
}

static void reallyExit(void) {
    thread nxt;

    RoundRobin->remove(running_th);
    remove_thread(running_th);
    free(running_th->stack);
    free(running_th);
    
    /* Get the next LWP to run. */
    nxt = RoundRobin->next();
    if(nxt == NULL) {
        /* No more LWP so restore main thread's rfile. */
        started = 0; /* We are effectively stopping the LWP process. */
        running_th = NULL;
        load_context(&main_rfile);
    }
    else {
        /* Switch to nxt's rfile. */
        running_th = nxt;
        load_context(&nxt->state);
    }
    return; /* We never get here. */
}


/* Yield the CPU to another LWP. This consists of the current LWP swapping 
 * rfiles with the next LWP. If there is none, return to main rfile. */
void lwp_yield(void) {
    thread nxt,cur;
    
    /* Get the next LWP to run from the scheduler. */
    nxt = RoundRobin->next();
    if(nxt == NULL) {
        /* No more LWP so restore main thread's rfile. */
        started = 0; /* We are effectively stopping the LWP process. */
        cur = running_th;
        running_th = NULL; 
        swap_rfiles(&cur->state,&main_rfile);
    }
    else {
        /* Switch to nxt's rfile. */
        cur = running_th;
        running_th = nxt;
        swap_rfiles(&cur->state,&nxt->state);
    }
    return; /* We never get here. */
}


/* Start the LWP system. This consists of saving the main file to a global,
 * switch rfiles to the next LWP if it exists, else return. */
void lwp_start(void) {
    thread nxt;

    if(process_count == 0) {
        return;
    }
    if(started == 1) {
        fprintf(stderr, "ERROR: Tried to start() an already started LWP.\n");
        return;
    }
    if(running_th != NULL) {
        fprintf(stderr, 
            "ERROR: Tried to start() but there was already a running LWP.\n");
        return;
    }

    /* Get the next LWP to run from the scheduler. */
    nxt = RoundRobin->next(); 
    if(nxt == NULL) {
        /* No next thread to run, return to main. */
        return;
    }
    else {
        started = 1;
        running_th = nxt;
        /* Switch to nxt's rfile, saving the main rfile in main_rfile. */
        swap_rfiles(&main_rfile,&nxt->state);
    }
    return; /* We never get here. */
}


/* Stop the LWP system. Returns us to where lwp_start() was initially called. 
 * This consists of restoring the main thread's rfile (setting the stack
 * pointer) */
void lwp_stop(void) {
    thread cur;
    
    if(started == 0) {
        fprintf(stderr, "ERROR: Tried to stop() a non-started LWP.\n");
        return;
    }
    if(running_th == NULL) {
        fprintf(stderr, "ERROR: Tried to stop() but not a running LWP.\n");
        return;
    }

    started = 0;
    cur = running_th;
    running_th = NULL;
    /* Switch to main's rfile. */
    swap_rfiles(&cur->state,&main_rfile);
    return; /* We never get here. */
}




/* Install a new scheduling function */
void lwp_set_scheduler(scheduler fun) {
    thread temp_thread;
    /* If fun is NULL, return to Round Robin scheduling. */
    if(fun == NULL) {
        RoundRobin = &rr_publish;
        return;
    }

    while((temp_thread = RoundRobin->next()) != NULL) {
        fun->admit(temp_thread);
        RoundRobin->remove(temp_thread);     
    }

    RoundRobin = fun;
    return;
}


/* Find out what the current scheduler is */
scheduler lwp_get_scheduler(void) {
   return RoundRobin;   
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


static void add_thread(thread new_lwp) {
   /* Connect linked list of threads */
   if(!thread_head) {
      thread_head = new_lwp;      
   }
   else {
      /* Put the new process behind thread_head */
     new_lwp->next = thread_head;
     new_lwp->prev = thread_head->prev;
     (thread_head->prev)->next = new_lwp;
     thread_head->prev = new_lwp;
   }
}


static void remove_thread(thread lwp) {
   if(!thread_head) {
      fprintf(stderr, "No threads exists");
      exit(1);
   }
   if(!lwp) {
      fprintf(stderr, "Tried to remove a NULL thread...just returning");
      return;
   }
   if(lwp->next == lwp) {
      /* There is onlt one thread in the list */
      thread_head = NULL;
   }
   else {
      /* Unlink the thread in the list */
      if(lwp->tid == thread_head->tid) {
         /* Don't want thread_head to be NULL */
         thread_head = thread_head->next;
      }
      (lwp->prev)->next = lwp->next;
      (lwp->next)->prev = lwp->prev;
   }
}
