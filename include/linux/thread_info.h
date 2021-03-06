#ifndef _THREAD_INFO_H_
#define _THREAD_INFO_H_


#define THREAD_SIZE_ORDER   1
#define THREAD_SIZE         8192
#define THREAD_START_SP     (THREAD_SIZE - 8)

struct thread_info {
	unsigned long flags; /* thread_info flags */
	struct task_struct *task; /* main task structure */
};

extern struct thread_info current_thread;
static inline int test_ti_thread_flag(struct thread_info *ti,int flag)
{
	return test_bit(flag,(unsigned long *)&ti->flags);
}

#define current_thread_info() (&(current_thread))

#define test_thread_flags(flag)  \
	test_ti_thread_flag(current_thread_info(),flag)

#define TIF_MEMDIE   18      /* is termination due to OOM killer */

#endif
