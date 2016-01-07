#include "../../include/linux/kernel.h"
#include "../../include/linux/setup.h"
#include "../../include/asm/arch.h"
#include "../../include/asm/map.h"
#include "../../include/asm/mach-types.h"
#include "../../include/linux/page.h"

extern unsigned int memory_array0[CONFIG_BANK0_SIZE / BYTE_MODIFY];

/*
 * Setup boot params
 */
void __uboot setup_bootparams(void)
{
	struct tag *tags = (struct tag *)(unsigned long)(
		(unsigned char)(unsigned long)memory_array0 + 0x100);

	/* We can setup boot params here */

}

/*
 * Called from setup _arch to fixup any tags
 * passed in by the boot loader.
 */
static void __init smk6410_fixup(struct machine_desc *desc,
		struct tag *tag,char **cmdline,struct meminfo *mi) 
{
	mi->nr_banks = 1;
	mi->bank[0].start = CONFIG_BANK0_START;
	mi->bank[0].size  = CONFIG_BANK0_SIZE;

#ifdef CONFIG_BOTH_BANKS
	mi->bank[1].start = CONFIG_BANK1_START;
	mi->bank[1].size  = CONFIG_BANK1_SIZE;
	mi->nr_banks++;
#endif
}
/*
 * For this machine:
 * machine_desc: __mach_desc_SMDK6410
 * nr: MACH_TYPE_SMDK6410
 * name: "SMDK6410"
 */
MACHINE_START(SMDK6410,"SMDK6410")
	.boot_params = S3C64XX_PA_SDRAM + 0x100,
	.fixup = smk6410_fixup,
MACHINE_END





