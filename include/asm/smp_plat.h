#ifndef _SMP_PLAT_H_
#define _SMP_PLAT_H_
#include "../linux/cputype.h"
/*
 * Return true if we are running on a SMP platform.
 */
static inline bool is_smp(void)
{
#ifndef CONFIG_SMP
	return 0;
#else
	return 1;
#endif
}

/* All SMP configurations have the extended CPUID register */
static inline int tlb_ops_need_broadcast(void)
{
	if(!is_smp())
		return 0;

	return ((read_cpuid_ext(CPUID_EXT_MMFR3) >> 12) & 0xf) < 2;
}

#endif
