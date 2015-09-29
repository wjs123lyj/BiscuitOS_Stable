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
	if(base0 <= base1 && end0 > base1)
	{
		mm_debug("Overlap below\n");
		return 1;
	}
	else if(base1 <= base0 && end1 > base0)
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
void memblock_migrate(unsigned long idx,unsigned long base,unsigned long size)
{
	unsigned long i;

	for(i = memblock.reserved.cnt ; i > idx ; i--)
	{
		memblock.reserved.regions[i].base = memblock.reserved.regions[i - 1].base;
		memblock.reserved.regions[i].size = memblock.reserved.regions[i - 1].size;
	}
}
/*
 * Find alloc address
 */
unsigned long memblock_find_alloc(unsigned long start,unsigned long end,
		unsigned long base,unsigned long size)
{
	int i;
	
	base = end - size;

	for(i = memblock.reserved.cnt - 1 ; i >= 0 ; i--)
	{
		if(memblock_overlap(base,size,memblock.reserved.regions[i].base,
					memblock.reserved.regions[i].size) != 0)
		{
			mm_debug("Alloc overlap\n");
			base = memblock.reserved.regions[i].base - size;
			continue;
		} else
		{
			mm_debug("No overlap\n");
			return base;
		}
	}
	if(base < start)
		return MM_NOREGION;
	else
		return base;
}
/*
 * find base address for allocate
 */
unsigned long memblock_find_base(unsigned long start,unsigned long size)
{
	unsigned long end;
	unsigned long base = 0;
	int i;

	for(i = 0 ; i < memblock.memory.cnt ; i++)
	{
		unsigned long regbase,regsize;
		unsigned long topbase,topend;

		regbase = memblock.memory.regions[i].base;
		regsize = memblock.memory.regions[i].size;
		topend = regsize + regbase;

		if(size > regsize)
		{
			mm_err("No match size\n");
			continue;
		}

		topbase = max(start,regbase);
		end = topbase + size;
		if(end > topend)
		{
			mm_err("Overflow tail\n");
			continue;
		}
		mm_debug("TopRegions[%d][%p - %p]\n",i,(void *)topbase,
				(void *)topend);
		return memblock_find_alloc(topbase,topend,base,size);
	}
	return MM_NOREGION;
}
/*
 * Add region underlying opertion
 */
int memblock_add_region_core(struct memblock_type *type,unsigned long base,
			unsigned long size)
{
	int i,j;
	unsigned long end = base + size;

	end = base + size;
	j = type->cnt;
	for(i = 0; i < type->cnt ; i--)
	{
		if(type->regions[0].base == 0 &&
				type->regions[0].size == 0)
		{
			mm_debug("This is first reserved\n");
			type->regions[0].base = base;
			type->regions[0].size = size;
			break;
		}
		else if(base < (type->regions[i].base + 
					type->regions[i].size))
		{
			mm_debug("o\n");
			memblock_migrate(i,base,end);
			type->regions[i].base = base;
			type->regions[i].size = size;
			type->cnt++;
			break;
		} else
		{
			mm_debug("Search loop\n");
		}
	}
	if(j == type->cnt && j != 1)
	{
		mm_debug("Add tail\n");
		type->regions[j].base = base;
		type->regions[j].size = size;
		type->cnt++;
	}
#ifdef DEBUG
	for(i = 0 ; i < memblock.reserved.cnt; i++)
	{
		mm_debug(">>>Region[%d][%p - %p]\n",i,
				(void *)type->regions[i].base,
				(void *)(type->regions[i].base + 
					type->regions[i].size));
	}
	mm_debug("======================================\n");
#endif
}
/*
 * Add region
 */
int memblock_add_region(unsigned long size)
{
	unsigned long base = 0;
	int i;

	base = memblock_find_base(0,size);
	if(base == MM_NOREGION)
		mm_err("Cant add region\n");
	mm_debug("Base %p\n",(void *)base);
	memblock_add_region_core(&memblock.reserved,base,size);
	return 0;
}
/*
 * bootmem_init.
 */
void bootmem_init(void)
{
	unsigned long min,max;
	int i;

	calculate_limit(&min,&max);

	for(i = 0 ; i < 50 ; i++)
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
