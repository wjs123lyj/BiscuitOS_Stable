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
#include "../include/linux/mmu.h"
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
}

int main()
{
	unsigned long addr = 0x5fffffff;
	unsigned int *p;

	start_kernel();
	
	p = phys_to_mem(addr);
	*p = 0x12345678;
	M_show(addr,addr + 10);
	printf("Hello World\n");
	return 0;
}
