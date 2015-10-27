#include "../../include/linux/kernel.h"
#include "../../include/linux/setup.h"
#include "../../include/linux/page.h"
#include "../../include/linux/debug.h"
/*
 * Initialize memblock of ARM
 */
void __init arm_memblock_init(struct meminfo *mi)
{
	int i;

	/*
	 * First initizalize the memblock.
	 */
	memblock_init();
	/*
	 * Add all region of memblock.memory.
	 */
	for(i = 0 ; i < mi->nr_banks ; i++)
		memblock_add(mi->bank[i].start,mi->bank[i].size);
	/*
	 * Add reserved region for swapper_pg_dir.
	 */
	arm_mm_memblock_reserve();
	/*
	 * calculate the total size of memblock.memory.regions
	 */
	memblock_analyze();
	/*
	 * Dump all information of memory and reserved.
	 */
	memblock_dump_all();
}
/*
 * Initilize the early parment.
 */
void early_parment(void)
{
	/*
	 * Initilize the VMALLOC_AREA.
	 */
	early_vmalloc();
}
/*
 * Find the normal memory limit.
 */
void __init find_limits(unsigned int *min,unsigned int *max_low,
		unsigned int *max_high)
{
	struct meminfo *mi = &meminfo;
	int i;
	
	*min = 0xFFFFFFFF;
	*max_low = *max_high = 0;

	for_each_bank(i,mi)
	{
		struct membank *bank = &mi->bank[i];
		unsigned int start,end;

		start = bank_pfn_start(bank);
		end   = bank_pfn_end(bank);

		if(*min > start)
			*min = start;
		if(*max_high < end)
			*max_high = end;
		if(bank->highmem)
			continue;
		if(*max_low < end)
			*max_low = end;
	}
}
/*
 * Bootmem init
 */
void __init bootmem_init(void)
{
	unsigned int min,max_low,max_high;

	max_low = max_high = 0;

	/*
	 * Get the limit of lowmem and highmem.
	 */
	find_limits(&min,&max_low,&max_high);
	/*
	 * Init all arm bootmem.
	 */
	arm_bootmem_init(min,max_low);
	/*
	 * Free all bootmem.
	 */
	arm_bootmem_free(min,max_low,max_high);

}







