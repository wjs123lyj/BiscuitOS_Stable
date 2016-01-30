#ifndef _CPUMASK_H_
#define _CPUMASK_H_

#include "linux/bitops.h"
#include "linux/threads.h"
#include "linux/bitmap.h"

typedef struct cpumask {DECLARE_BITMAP(bits,NR_CPUS);} cpumask_t;

#define cpu_online_mask 0

extern const struct cpumask *const cpu_possible_mask;


#define for_each_cpu(cpu,mask)         \
	for((cpu) = 0 ; (cpu) < 1 ; (cpu)++,(void)mask)

#define for_each_online_cpu(cpu)  for_each_cpu((cpu),cpu_online_mask)
#define for_each_possible_cpu(cpu)  for_each_cpu((cpu),cpu_possible_mask)

#define nr_cpu_ids    1
#define num_online_cpus()   1U
#define cpu_possible(cpu)  ((cpu) == 0)


#define CPU_MASK_LAST_WORD BITMAP_LAST_WORD_MASK(NR_CPUS)

#define CPU_BITS_ALL     \
{                              \
	[BITS_TO_LONGS(NR_CPUS) - 1] = CPU_MASK_LAST_WORD  \
}

#define num_possible_cpus()    1U

#define nr_cpumask_bits NR_CPUS

/**
 * to_cpumask - convert an NR_CPUS bitmap to a struct cpumask *
 * @bitmap:the bitmap
 *
 * There are a few places where cpumask_var_t isn't approprivate and
 * static cpumasks must be used(eg.very early boot),yet we don't
 * expose the definition of 'struct cpumask'
 *
 * This does the conversion,and can be used as a constant initializer.
 */
#define to_cpumask(bitmap)      \
	((struct cpumask *)(1 ? (bitmap)        \
			: (void *)sizeof(__check_is_bitmap(bitmap))))

static inline int __check_is_bitmap(unsigned int *bitmap)
{
	return 1;
}

/**
 * cpumask_bits - get the bits in a cpumask
 * @maskp: the struct cpumask *
 *
 * You should only assume nr_cpu_ids bits of this mask are valid.This is
 * a macro so it's const-correct.
 */
#define cpumask_bits(maskp) ((maskp)->bits)

/**
 * cpumask_scnprintf - print a cpumask into a string as comma-separated hex
 * @buf: the buffer to sprintf into
 * @len: the length of the buffer
 * @srcp:the cpumask to print
 *
 * If len is zero,return zero.Otherwise returns the length of the
 * (nul-terminated)@buf string.
 */
static inline int cpumask_scnprintf(char *buf,int len,
		const struct cpumask *srcp)
{
	return bitmap_scnprintf(buf,len,cpumask_bits(srcp),nr_cpumask_bits);
}



#endif
