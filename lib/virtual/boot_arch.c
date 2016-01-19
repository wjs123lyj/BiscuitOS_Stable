#include "../include/linux/kernel.h"
#include "../include/linux/setup.h"
#include "../include/linux/debug.h"

unsigned int MEM0_OFFSET;
#ifdef CONFIG_BOTH_BANKS
unsigned int MEM1_OFFSET;
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
 * Initialize the virtual physical memory.
 */
void __uboot virtual_memory_init(void)
{
	/*
	 * Get dynamic virtual address offset.
	 */
	memset(memory_array0,0,sizeof(CONFIG_BANK0_SIZE));
	MEM0_OFFSET = (unsigned long)(
			(unsigned long)(unsigned int *)CONFIG_BANK0_START - 
			(unsigned long)(unsigned long)memory_array0);
#ifdef CONFIG_BOTH_BANKS
	memset(memory_array1,0,sizeof(CONFIG_BANK1_SIZE));
	MEM1_OFFSET = (unsigned long)(
			(unsigned long)(unsigned int *)CONFIG_BANK1_START -
			(unsigned long)(unsigned long)memory_array1);
#endif

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
	phys = (void *)((unsigned long)addr - (unsigned long)MEM0_OFFSET);
#else
	if(addr < CONFIG_BANK0_START || 
			(addr > MAX_BANK0_PHYS_ADDR && addr < CONFIG_BANK1_START ) ||
			addr > MAX_BANK1_PHYS_ADDR)
	{
		mm_err("Error:Bad_physical address\n");
		return NULL;
	}
	if(addr < CONFIG_BANK1_START)
		phys = (void *)((unsigned long)addr - (unsigned long)MEM0_OFFSET);
	else
		phys = (void *)((unsigned long)addr - (unsigned long)MEM1_OFFSET);
#endif
	return phys;
}
/*
 * Virtual memory address turn to physical address.
 */
phys_addr_t mem_to_phys(void *ad)
{
	unsigned long addr = (unsigned long)(unsigned int *)ad;
	unsigned long base0,end0;
	phys_addr_t phys_addr;
#ifdef CONFIG_BOTH_BANKS
	unsigned long base1,end1;

	base1 = (unsigned long)(unsigned int *)memory_array1;
	end1  = base1 + CONFIG_BANK1_SIZE;
#endif
	base0 = (unsigned long)(unsigned int *)memory_array0;
	end0  = base0 + CONFIG_BANK0_SIZE;

#ifdef CONFIG_BOTH_BANKS
	if(addr < base0 || addr > end1 ||
			(addr > end0 && addr < base1))
	{
		mm_err("Err:Bad_memory address\n");
		return 0;
	}
	/*
	 * The memory address belong to BANK2.
	 */
	if(addr > end0)
	{
		phys_addr = addr + (unsigned long)MEM1_OFFSET;
	} else
	{
		phys_addr = addr + (unsigned long)MEM0_OFFSET;
	}
#else
	if(addr < base0 || addr > end0)
	{
		mm_err("Err:Bad_memory address\n");
		return 0;
	}
	phys_addr = addr + (unsigned long)MEM0_OFFSET;
#endif
	return phys_addr;
}
