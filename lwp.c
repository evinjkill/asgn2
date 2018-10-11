#include "lwp.h"
#include <stdint.h>

static struct scheduler rr_publish = {NULL, NULL, rr_admit, rr_remove, rr_next}
static unsigned int process_count = 0;
/* List will be linked in a circle, but need a point of reference */
static thread thread_head = NULL;

/* Create a new LWP */
tid_t lwp_create(lwpfun function, void *argument, size_t stack_size) {
   unsigned long *stack;
   context *new_lwp;
   new_lwp->stacksize = stack_size;

   /* Assign process id based on process count */
   new_lwp->tid = (tid_t)++process_count;

   /* Allocate space for the stack 
    * TODO: Might have to change stack size to be in words, not bytes */
   stack = (unsigned long*)malloc(stack_size);

   /* Stack in context refers to the base of the stack */
   new_lwp->stack = stack;

   /* Move stack_pointer to the "top" of the stack */
   stack += stack_size;

   /* TODO: Update and save rfiles to this context */
   
   
   /* TODO: Figure out what to do with schedulers in new_lwp */

   new_lwp->next = NULL;
   new_lwp->prev = NULL;

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

}


/* Return thread ID of the calling LWP */
void  lwp_exit(void) {

}


/* Terminates the calling LWP */
tid_t lwp_gettid(void) {

}


/* Yield the CPU to another LWP */
void  lwp_yield(void) {

}


/* Start the LWP system */
void  lwp_start(void) {

}


/* Stop the LWP system */
void  lwp_stop(void) {

}


/* Install a new scheduling function */
void  lwp_set_scheduler(scheduler fun) {

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
