#ifndef _NOTIFIER_H_
#define _NOTIFIER_H_

#define CPU_DEAD     0x0007 /* CPU (unsigned)v dead */

#define CPU_TASKS_FROZEN  0x0010

#define CPU_DEAD_FROZEN (CPU_DEAD | CPU_TASKS_FROZEN)


#define NOTIFY_OK    0x0001     /* Suits me */

struct notifier_block {
	int (*notifier_call)(struct notifier_block *,unsigned long,void *);
	struct notifier_block *next;
	int priority;
};

#endif
