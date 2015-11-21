#ifndef _POISON_H_
#define _POISON_N_

/******* mm/debug-pagealloc.c *******/
#define PAGE_POISON   0xaa

#define POISON_POINTER_DELTA 0
/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances,used to verify that nobody uses
 * non-initialized list entries.
 */
#define LIST_POISON1 ((void *)0x00100100 + POISON_POINTER_DELTA)
#define LIST_POISON2 ((void *)0x00200200 + POISON_POINTER_DELTA)
#endif
