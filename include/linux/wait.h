#ifndef _WAIT_H_
#define _WAIT_H_
#include "lockdep.h"
#include "list.h"
#include "spinlock_types.h"


typedef struct __wait_queue wait_queue_t;
typedef int (*wait_queue_func_t)(wait_queue_t *wait,
		unsigned mode,int flags,void *key);

struct __wait_queue {
	unsigned int flags;
#define WQ_FLAG_EXCLUSIVE  0x01
	void *private;
	wait_queue_func_t func;
	struct list_head task_list;
};

struct __wait_queue_head {
	spinlock_t lock;
	struct list_head task_list;
};
typedef struct __wait_queue_head wait_queue_head_t;


extern void __init_waitqueue_head(wait_queue_head_t *q,struct lock_class_key *);
#define init_waitqueue_head(q)              \
	do {                                     \
	       static struct lock_class_key __key; \
	                                           \
	       __init_waitqueue_head((q),&__key);   \
	} while(0)


#endif
