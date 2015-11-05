#ifndef _MM_TYPE_H_
#define _MM_TYPE_H_
#include "pgtable.h"
#include "config.h"
#include "sched.h"
/*
 * mm_struct 
 */
struct mm_struct {
	pgd_t *pgd;
#ifdef CONFIG_MM_OWNER
	/*
	 * "owner" points to a task that is regarded as the canonical
	 * user/owner of this mm.All of the follow must be true true in
	 * order for it to be changed:
     *
	 * current == mm->owner
	 * current->mm != mm
	 * new_owner->mm == mm
	 * new_owner->alloc_lock is held
	 */
	struct task_struct *owner;
#endif
};

#endif
