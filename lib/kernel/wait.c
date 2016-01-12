#include "../../include/linux/kernel.h"
#include "../../include/linux/wait.h"
#include "../../include/linux/spinlock.h"
#include "../../include/linux/list.h"


void __init_waitqueue_head(wait_queue_head_t *q,struct lock_class_key *key)
{
	spin_lock_init(&q->lock);
	lockdep_set_class(&q->lock,key);
	INIT_LIST_HEAD(&q->task_list);
}
