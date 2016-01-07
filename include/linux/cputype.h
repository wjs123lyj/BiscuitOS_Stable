#ifndef _CPUTYPE_H_
#define _CPUTYPE_H_   1

#define CPUID_ID            0
#define CPUID_CACHETYPE     1
#define CPUID_TCM           2
#define CPUID_TLBTYPE       3

#define CPUID_EXT_PFR0
#define CPUID_EXT_PFR1
#define CPUID_EXT_DFR0
#define CPUID_EXT_AFR0
#define CPUID_EXT_MMFR0  "c1,4"
#define CPUID_EXT_MMFR1
#define CPUID_EXT_MMFR2
#define CPUID_EXT_MMFR3  "c1,7"
#define CPUID_EXT_ISAR0
#define CPUID_EXT_ISAR1
#define CPUID_EXT_ISAR2
#define CPUID_EXT_ISAR3
#define CPUID_EXT_ISAR4
#define CPUID_EXT_ISAR5

static inline unsigned int read_cpuid(int reg)
{
	extern unsigned int CP15_C00;

	if(reg == CPUID_ID)
		return CP15_C00;
	else if(reg == CPUID_CACHETYPE)
		return 0x1d152152;
	else
		return 0;
}
static inline unsigned int read_cpuid_ext(char *reg)
{
	if(strcmp("c1,4",reg) == 0)
		return 0x01130003;
	else if(strcmp("c1,7",reg) == 0)
		return 0;
	else
		return 0;
}

/*
 * The CPU ID never changes at run time,so we might as well tell the
 * compiler that it's constant.Use this function to read the CPU ID
 * rather than directly reading processor_id or read_cpuid() directly.
 */
static inline unsigned int read_cpuid_id(void)
{
	return read_cpuid(CPUID_ID);
}
static inline unsigned int read_cpuid_cachetype(void)
{
	return read_cpuid(CPUID_CACHETYPE);
}
/*
 * Intel's XScale3 core supports some v6 features.
 * but advertises itself as v5 as it does not support the v6 ISA.For
 * this reason ,we need a way to explicitly test for this type of CPU.
 */
static inline int cpu_is_xsc3(void)
{
	unsigned int id;

	id = read_cpuid_id() & 0xffffe000;
	/* It covers both intel ID and Marvel ID */
	if((id == 0x69056000) || (id == 0x56056000))
		return 1;

	return 0;
}

#if !defined(CONFIG_CPU_XSCALE) && !defined(CONFIG_CPU_XSC3)
#define cpu_is_xscale()    0
#else
#define cpu_is_xscale()    1
#endif
#endif
