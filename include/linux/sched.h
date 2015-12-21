#ifndef _SCHED_H_
#define _SCHED_H_

#define TASK_COMM_LEN  16

struct task_struct {
	unsigned long state;
	unsigned int flags;
	int prio;
	struct reclaim_state *reclaim_state;
	char comm[TASK_COMM_LEN];
};

#define MAX_USER_RT_PRIO    100
#define MAX_RT_PRIO         MAX_USER_RT_PRIO

static inline int rt_prio(int prio)
{
	if(unlikely(prio < MAX_RT_PRIO))
		return 1;
	return 0;
}
static inline int rt_task(struct task_struct *p)
{
	return rt_prio(p->prio);
}

/*
 * Per process flags.
 */
#define PF_MEMALLOC      0x000000800  /* Allocating memory */

#endif
