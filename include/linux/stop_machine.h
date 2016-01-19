#ifndef _STOP_MACHINE_H_
#define _STOP_MACHINE_H_
#include "irqflags.h"

static inline int __stop_machine(int (*fn)(void *),void *data,
		struct cpumask *cpus)
{
	int ret;
	local_irq_disable();
	ret = fn(data);
	local_irq_enable();
	return ret;
}

static inline int stop_machine(int (*fn)(void *),void *data,
		struct cpumask *cpus)
{
	return __stop_machine(fn,data,cpus);
}

#endif
