#include "../../include/linux/memblock.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/page.h"
#include "../../include/linux/kernel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	return (end - start + 7) >> 3;
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
	memblock.memory.max = MAX_REGIONS;
	memblock.memory.limit = 3 * MAX_REGIONS;
	memblock.reserved.regions = initrd_reserve;
	memblock.reserved.cnt = 1;
	memblock.reserved.max = MAX_REGIONS;
	memblock.reserved.limit = 3 * MAX_REGIONS;
	/*
	 * Init Region.
	 */
	memblock.memory.regions[0].base = buddy_memory.start_pfn;
	memblock.memory.regions[0].size = buddy_memory.end_pfn - buddy_memory.start_pfn;
	memblock.reserved.regions[0].base = 0;
	memblock.reserved.regions[0].size = 0;

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
		return 1;
	else if(base1 <= base0 && end1 > base0)
		return -1;
	else
		return 0;
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
	struct memblock_type *type = &memblock.reserved;
	
	base = end - size;

	for(i = type->cnt - 1 ; i >= 0 ; i--)
	{
		if(memblock_overlap(base,size,type->regions[i].base,
					type->regions[i].size) != 0)
		{
			base = type->regions[i].base - size;
			continue;
		} else
		{
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
	struct memblock_type *type = &memblock.memory;

	for(i = 0 ; i < type->cnt ; i++)
	{
		unsigned long regbase,regsize;
		unsigned long topbase,topend;

		regbase = type->regions[i].base;
		regsize = type->regions[i].size;
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
unsigned long memblock_add_region_core(struct memblock_type *type,unsigned long base,
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
			type->regions[0].base = base;
			type->regions[0].size = size;
			break;
		}
		else if(base < (type->regions[i].base + 
					type->regions[i].size))
		{
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

	return base;
}
/*
 * Expand array of regions.
 */
int memblock_double_array(struct memblock_type *type)
{
	struct memblock_region *region = NULL;
	struct memblock_region **new;

	if(type->max + MAX_REGIONS > type->limit)
	{
		mm_err("Can't expand the region\n");
		return MM_NOEXPAND;
	}
	
	region = (struct memblock_region *)malloc(
			sizeof(struct memblock_region) * (MAX_REGIONS +
				type->max));
	memcpy(region,type->regions,type->max * sizeof(struct memblock_region));	
	memset(region + type->max,0,MAX_REGIONS);
	type->max += MAX_REGIONS;
	type->regions = region;

	return 0;
}
/*
 * Add region
 */
unsigned long memblock_add_region(struct memblock_type *type,unsigned long size)
{
	unsigned long base = 0;
	int i;

	if(type->cnt == type->max)
		if(type->max >= type->limit)
		{
			mm_err("Can't add region\n");
			return MM_NOEXPAND;
		} else
		{
			mm_debug("Double array\n");
			memblock_double_array(type);
		}

	base = memblock_find_base(0,size);
	if(base == MM_NOREGION)
	{
		mm_err("Cant add region\n");
		return MM_NOREGION;
	}
	return memblock_add_region_core(&memblock.reserved,base,size);
}
/*
 * Free region.
 */
int memblock_remove_region(struct memblock_type *type,unsigned long base,
		unsigned long size)
{
	int i;

	mm_debug("Remove base %p\n",(void *)base);
	for(i = 0 ; i < type->cnt ; i++)
	{
		unsigned long regbase = type->regions[i].base;
		unsigned long regend  = type->regions[i].size + regbase;

		if(base >= regbase && base < regend)
			break;
	}
	if(i == type->cnt)
	{
		mm_err("Can't match region in [%s]\n",__FUNCTION__);
		return MM_NOREGION;
	}
	for(; i < type->cnt ; i++)
	{
		type->regions[i].base = type->regions[i + 1].base;
		type->regions[i].size = type->regions[i + 1].size;
	}
	type->regions[i].base = 0;
	type->regions[i].size = 0;
	type->cnt--;
	
	return 0;
}
/*
 * Allocate a new region for bitmap.
 */
unsigned long bootmem_bitmap_region(unsigned long pages)
{
	return memblock_add_region(&memblock.memory,pages);
}
/*
 * bootmem_init.
 */
void bootmem_init(void)
{
	unsigned long min,max;
	unsigned long bytes,pages;
	unsigned long bitmap;

	calculate_limit(&min,&max);

	bytes = bootmem_bitmap_bytes(min,max);
	pages = bootmem_bitmap_pages(bytes);
	bitmap = bootmem_bitmap_region(pages);

	mm_debug("Pages %p\n",(void *)bitmap);

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
