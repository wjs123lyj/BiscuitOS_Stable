#ifndef _PERCPU_DEFS_H_
#define _PERCPU_DEFS_H_

/*
 * Variant on the per-CPU variable declaration/definition theme used for
 * ordinary per-CPU variables.
 */
#define DECLARE_PER_CPU_SECTION(type,name,sec)    \
	__typeof__(type) name

#define DECLARE_PER_CPU(type,name)     \
	DECLARE_PER_CPU_SECTION(type,name,"")

#define DEFINE_PER_CPU(type,name)      \
	DECLARE_PER_CPU_SECTION(type,name,"")

#endif
