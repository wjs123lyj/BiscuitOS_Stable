#include "../../include/linux/memblock.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/page.h"
#include <stdio.h>
#include <stdlib.h>
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
static unsigned long bootmem_bitmap_bytes(unsigned long start,unsigned long end)
{
	return (end - start + 7) >> 8;
}
/*
 * Calculate the bit that store pfn.
 */
static unsigned long bootmem_bitmap_pages(unsigned long bytes)
{
	return (bytes + PAGE_SIZE - 1) >> PAGE_SHIFT;
}
