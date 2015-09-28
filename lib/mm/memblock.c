#include "../../include/linux/memblock.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/page.h"
#include "../../include/linux/kernel.h"
#include <stdio.h>
#include <stdlib.h>

struct memblock_region initrd_memory[MAX_REGIONS];
struct memblock_region initrd_reserve[MAX_REGIONS];
struct memblock memblock;
struct memory buddy_memory = {
	.start_pfn = 0x00050000,
	.end_pfn   = 0x00060000,
};
/*
 * Get pfn of start and end.
 */
void calculate_limit(unsigned long *min,unsigned long *max_low)
{

	*min = buddy_memory.start_pfn;
	*max_low   = buddy_memory.end_pfn;

	mm_debug("Calculate limit Region[%p - %p]\n",
			(void *)*min,(void *)*max_low);
	/*
	 * Domain check.
	 */
	if(*min >= *max_low)
	{
		mm_debug("Bad size\n");
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
	memblock.memory.regions[0].base = buddy_memory.start_pfn;
	memblock.memory.regions[0].size = buddy_memory.end_pfn - buddy_memory.start_pfn;
	memblock.reserved.regions[0].base = 0;
	memblock.reserved.regions[0].size = 0;

	mm_debug("Memblock_init Meory.region[%p - %p]\n",
			(void *)memblock.memory.regions[0].base,
			(void *)(memblock.memory.regions[0].base +
			memblock.memory.regions[0].size));
	mm_debug("Membloc_init Reserved.region[%p - %p]\n",
			(void *)memblock.reserved.regions[0].base,
			(void *)(memblock.reserved.regions[0].base +
				memblock.reserved.regions[0].size));
	return 0;
}
/*
 * Overlap
 * base1 and size1 is flags
 * 1: below
 * -1: after
 * 0: No overlap.
 */
int memblock_overlap(unsigned long base0,unsigned long size0,
		unsigned long base1,unsigned long size1)
{
	unsigned long end0,end1;

	end0 = base0 + size0;
	end1 = base1 + size1;
	if(base0 < base1 && end0 > base1)
	{
		mm_debug("Overlap below\n");
		return 1;
	}
	else if(base1 < base0 && end1 > base0)
	{
		mm_debug("Overlap after\n");
		return -1;
	}
	else
	{
		mm_debug("No overlap\n");
		return 0;
	}
}
/*
 * memblock region adjust
 */
void memblock_adjust(unsigned long idx,unsigned long base,unsigned long size)
{
	unsigned long i;

	for(i = memblock.reserved.cnt ; i > idx ; i--)
	{
		memblock.reserved.regions[i].base = memblock.reserved.regions[i - 1].base;
		memblock.reserved.regions[i].size = memblock.reserved.regions[i - 1].size;
	}
	memblock.reserved.cnt++;
}
/*
 * Add region underlying opertion
 */
int __memblock_add_region(unsigned long starts,unsigned long ends,
			unsigned long base,unsigned long size)
{
	unsigned long i;
	unsigned long end = base + size;

	mm_debug("FRegion[%p - %p]\n",
			(void *)base,(void *)end);

	mm_debug("cnt %ld\n",memblock.memory.cnt);

	for(i = memblock.reserved.cnt - 1; i >= 0; i--)
	{
		mm_debug("CRegions[%ld][%p - %p]\n",
				i,(void *)memblock.reserved.regions[i].base,
				(void *)(memblock.reserved.regions[i].base +
					memblock.reserved.regions[i].size));
		if(memblock_overlap(base,size,memblock.reserved.regions[i].base,
					memblock.reserved.regions[i].size) != 0)
		{
			base = memblock.reserved.regions[i].base - size;
			mm_debug("Reserved regions[%p - %p]\n",
				(void *)memblock.reserved.regions[i].base,
				(void *)(memblock.reserved.regions[i].base + 
					memblock.reserved.regions[i].size));
		} else
		{
			mm_debug("This is test\n");
			break;
		}
	}
	mm_debug("NewRegion[%p - %p]\n",(void *)base,
			(void *)(base + size));
	end = base + size;
	for(i = memblock.reserved.cnt - 1 ; i >= 0 ; i--)
	{
		if(memblock.reserved.regions[0].base == 0 &&
				memblock.reserved.regions[0].size == 0)
		{
			memblock.reserved.regions[0].base = base;
			memblock.reserved.regions[0].size = size;
		}
		if(end < memblock.reserved.regions[i].base &&
				base > (memblock.reserved.regions[i - 1].base + 
					memblock.reserved.regions[i - 1].size))
		{
			memblock_adjust(i,base,end);
			memblock.reserved.regions[i].base = base;
			memblock.reserved.regions[i].size = size;
			memblock.reserved.cnt++;
			break;
		} else
		{
			memblock.reserved.regions[i + 1].base = base;
			memblock.reserved.regions[i + 1].size = size;
			memblock.reserved.cnt++;
			break;
		}
	}
#ifdef DEBUG
	for(i = 0 ; i < memblock.reserved.cnt; i++)
	{
		mm_debug("Region[%p - %p]\n",
				(void *)memblock.reserved.regions[i].base,
				(void *)(memblock.reserved.regions[i].base + 
					memblock.reserved.regions[i].size));
	}
#endif
}
/*
 * Add region
 */
int memblock_add_region(unsigned long size)
{
	unsigned long base = 0;
	unsigned long i;
	unsigned long regbase = 0,regend = 0;

	for(i = 0 ; i < memblock.memory.cnt ; i++)
	{
		mm_debug("Findregion[%ld] End[%p] rivee end[%p]\n",i,
				(void *)(memblock.memory.regions[i].base +
					memblock.memory.regions[i].size),
				(void *)regend);
		regend = max(regend,memblock.memory.regions[i].base + 
				memblock.memory.regions[i].size);
	}
	base = regend - size;
	regbase = base;

	for(i = 0 ; i < memblock.memory.cnt ; i++)
	{
		mm_debug("Findregion[%ld] Base[%p] regb[%p]\n",
				i,(void *)base,
				(void *)memblock.memory.regions[i].base);
		regbase = min(regbase,memblock.memory.regions[i].base);
	}
	
	mm_debug("Reg[%p - %p]\n",(void *)regbase,(void *)regend);
	__memblock_add_region(regbase,regend,base,size);
	return 0;
}
/*
 * bootmem_init.
 */
void bootmem_init(void)
{
	unsigned long min,max;

	calculate_limit(&min,&max);

	memblock_add_region(0x80);

	memblock_add_region(0x80);
}
/*
 * ARCH init
 */
void arch_setup(void)
{
	unsigned long idx;

	memblock_init();

	bootmem_init();
}
