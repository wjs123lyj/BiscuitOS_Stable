#include "../../include/linux/memblock.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/page.h"
#include <stdio.h>
#include <stdlib.h>

struct memblock_region initrd_memory[MAX_REGIONS];
struct memblock_region initrd_reserve[MAX_REGIONS];
struct memblock memblock;
struct memory buddy_memory = {
	.start_pfn = 0x50000000,
	.end_pfn   = 0x60000000,
};
/*
 * Get pfn of start and end.
 */
void calculate_limit(unsigned long *min,unsigned long *max_low)
{

	*min = buddy_memory.start_pfn;
	*max_low   = buddy_memory.end_pfn;

	/*
	 * Domain check.
	 */
	if(*min >= *max_low)
	{
#ifdef DEBUG
		printf("Bad size\n");
#endif
		return;
	}
}
/*
 * Creat bitmap of all pages. 
 */
unsigned long bootmem_bitmap_bytes(unsigned long start,unsigned long end)
{
	return (end - start + 7) >> 8;
}
/*
 * Calculate the bit that store pfn.
 */
unsigned long bootmem_bitmap_pages(unsigned long bytes)
{
	return (bytes + PAGE_SIZE - 1) >> PAGE_SHIFT;
}
/*
 * Memblock init
 */
int memblock_init(void)
{
	/*
	 * Init Memory regions.
	 */
	memblock.memory.regions = initrd_memory;
	memblock.memory.cnt = 1;
	memblock.reserved.regions = initrd_reserve;
	memblock.reserved.cnt = 1;
	/*
	 * Init Region.
	 */
	memblock.memory.regions[0].base = 0;
	memblock.memory.regions[0].size = 0;
	memblock.reserved.regions[0].base = 0;
	memblock.reserved.regions[0].size = 0;

	return 0;
}



