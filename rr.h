/* This is a header file for the Round robin scheduler. Written by Ryan Kruger
 * and Evin Killian. */

#include "lwp.h"

#ifndef RRH
#define RRH

/* Prototypes */
void rr_init(void);
void rr_shutdown(void);
void rr_admit(thread);
void rr_remove(thread);
thread rr_next(void);

#endif
