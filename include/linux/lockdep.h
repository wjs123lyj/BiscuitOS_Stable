#ifndef _LOCKDEP_H_
#define _LOCKDEP_H_

/*
 * The class key takes no space if lockdep is disabled.
 */
struct lock_class_key {};

#define lockdep_trace_alloc(g)   do {} while(0)

#define lockdep_set_current_reclaim_state(g) do {} while(0)

#define lockdep_set_class(lock,key) do { (void)(key);} while(0)

#endif
