#include "../include/linux/memblock.h"
#include "../include/linux/debug.h"
#include "../include/linux/kernel.h"
#include "../include/linux/list.h"
#include "../include/linux/pgtable.h"
#include "../include/linux/mm_type.h"
#include "../include/linux/types.h"
#include "../include/linux/highmem.h"
#include "../include/linux/config.h"
#include "../include/linux/boot_arch.h"
#include "../include/linux/init_mm.h"
#include <stdio.h>
#include <stdlib.h>

//extern unsigned int top_mem;

static void start_kernel(void)
{

	/* Initialize virtual physical layer */
	virt_arch_init();
	/* Early parment */
	early_parment();
	/* ARM init memblock */
	arm_memblock_init(&meminfo);
	/* Initlize the paging table */
	paging_init();
    /* User debug */
	/* End debug */
	arch_init();
	M_show(virt_to_phys(swapper_pg_dir),virt_to_phys(swapper_pg_dir) + 0x40);
}

int main()
{
//	start_kernel();
	virt_arch_init();
	printf("Hello World\n");
	return 0;
}
