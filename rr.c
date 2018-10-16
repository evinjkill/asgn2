/* This is rr.c, a Round Robin scheduling program. It provides functions based
 * on the scheduler template. Written by Ryan Kruger and Evin Killian. */

#include "rr.h"
#include "lwp.h"
#include <stdlib.h>
#include <stdio.h>

static thread thread_head = NULL;
static thread running_th = NULL;

void rr_init(void) {
    /* Not implemented for RR. */
    return;
}


void rr_shutdown(void) {
    /* Not implemented for RR. */
    return;
}


/* Adds a thread to the scheduler's list of threads. */
void rr_admit(thread new) {

    /* Insert a node into the circular list. */
    if(thread_head == NULL) {
        thread_head = new;
        thread_head->s_next = thread_head;
        thread_head->s_prev = thread_head;
    }
    else {
        new->s_next = thread_head;
        new->s_prev = thread_head->s_prev;
        (thread_head->s_prev)->s_next = new;
        thread_head->s_prev = new;
    }
    return;
}


/* Removes a thread from the scheduler's list of threads. */
void rr_remove(thread victim) {

    if(victim == NULL) {
        fprintf(stderr, "ERROR: Tried to remove() a NULL thread.\n");
        return;
    }
    if(thread_head == NULL) {
        fprintf(stderr, 
            "ERROR: Tried to remove() a thread from an empty list.\n");
        return;
    }
    if(victim->s_next == victim) {
        thread_head = NULL;
        running_th = NULL;
    }
    else {
        if(victim->tid == thread_head->tid) {
            thread_head = thread_head->s_next;
        }
        running_th = running_th->s_prev;
        (victim->s_prev)->s_next = victim->s_next;
        (victim->s_next)->s_prev = victim->s_prev;
    }
    return;
}


/* Returns the next thread in the list. This will loop around the list in
 * Round Robin order. */
thread rr_next(void) {
    if(thread_head == NULL) {
        return NULL;
    }
    if(running_th == NULL)
        running_th = thread_head;
    else
        running_th = running_th->s_next;
    return running_th;
}
