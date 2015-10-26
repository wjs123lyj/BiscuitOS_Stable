#include "../include/linux/config.h"
#include "../include/linux/memblock.h"
#include "../include/linux/boot_arch.h"
#include "../include/linux/setup.h"
#include "../include/linux/debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
 * boot memory bank.
 */
struct meminfo meminfo = {
	.nr_banks = 0,
};
/*
 * Get dynamic offset between physical address and virtual memory address.
 */
static unsigned int MEM0_OFFSET;
#ifdef CONFIG_BOTH_BANKS
static unsigned int MEM1_OFFSET;
#endif
/*
 * We use an unsigned int array to simulate the first physical memory.
 */
unsigned int memory_array0[CONFIG_BANK0_SIZE / BYTE_MODIFY];
/*
 * We use an unsigned int array to simulate the second physical memory.
 */
#ifdef CONFIG_BOTH_BANKS
unsigned int memory_array1[CONFIG_BANK1_SIZE / BYTE_MODIFY];	
#endif
/*
 * Initialize the system physical memory information.
 */
static void boot_init_meminfo(void)
{
	/*
	 * Get dynamic virtual address offset.
	 */
	memset(memory_array0,0,sizeof(CONFIG_BANK0_SIZE));
	MEM0_OFFSET = (unsigned int)((unsigned int)CONFIG_BANK0_START -
			(unsigned int)memory_array0);
#ifdef CONFIG_BOTH_BANKS
	memset(memory_array1,0,sizeof(CONFIG_BANK1_SIZE));
	MEM1_OFFSET = (unsigned int)((unsigned int)CONFIG_BANK1_START -
			(unsigned int)memory_array1);
#endif
	/*
	 * Setup meminfo.
	 */
	meminfo.bank[0].start = CONFIG_BANK0_START;
	meminfo.bank[0].size  = CONFIG_BANK0_SIZE;
	meminfo.bank[0].highmem = 0;
	meminfo.nr_banks++;
#ifdef CONFIG_BOTH_BANKS
	meminfo.bank[1].start = CONFIG_BANK1_START;
	meminfo.bank[1].size  = CONFIG_BANK1_SIZE;
	meminfo.bank[1].highmem = 0;
	meminfo.nr_banks++;
#endif
	mm_debug("MEM0_OFFSET %08x\n",MEM0_OFFSET);
}
/*
 * Physical address turn to virtual memory address.
 */
void *phys_to_mem(phys_addr_t addr)
{
	void *phys;

#ifndef CONFIG_BOTH_BANKS 
	if(addr < CONFIG_BANK0_START || addr > MAX_BANK0_PHYS_ADDR)
	{
		mm_err("Error:Bad physical address\n");
		return NULL;
	}
	phys = (void *)(addr - MEM0_OFFSET);
#else
	if(addr < CONFIG_BANK0_START || 
			(addr > MAX_BANK0_PHYS_ADDR && addr < CONFIG_BANK1_START ) ||
			addr > MAX_BANK1_PHYS_ADDR)
	{
		mm_err("Error:Bad_physical address\n");
		return NULL;
	}
	if(addr < CONFIG_BANK1_START)
		phys = (void *)(addr - MEM0_OFFSET);
	else
		phys = (void *)(addr - MEM1_OFFSET);
#endif
	return phys;
}
/*
 * ARCH_INIT
 */
void virt_arch_init(void)
{
	boot_init_meminfo();
}
