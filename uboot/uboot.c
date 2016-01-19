#include "../include/linux/kernel.h"
#include "../include/linux/debug.h"

extern void __uboot virtual_memory_init(void);
extern void __uboot setup_bootparams(void);
extern void __uboot init_CP15(void);
/*
 * This file is used to simulate the uboot that pass tag to kernel.
 */
extern struct machine_desc __mach_desc_SMDK6410;
void u_boot_start(void)
{
	u32 value = (u32)(unsigned long)&__mach_desc_SMDK6410;

	init_CP15();
	set_cr1(value);

	virtual_memory_init();

	setup_bootparams();
}
