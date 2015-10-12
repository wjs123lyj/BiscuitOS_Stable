#include "../../include/linux/memblock.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/page.h"
#include "../../include/linux/kernel.h"
#include "../../include/linux/types.h"
#include <malloc.h>
#include <string.h>

struct memblock_region default_memory[MAX_REGIONS];
struct memblock_region default_reserved[MAX_REGIONS];
struct memory memory = {
	.start = 0x50000000,
	.end   = 0x60000000,
};
struct memblock memblock = {
	.current_limit = MAX_REGIONS,
};

unsigned long memory_array[67108864];
unsigned long PHYS_OFFSET;

struct pglist_data contig_pglist_data;

#define phys_to_virt(x) ((x) - PHYS_OFFSET)

static struct pglist_data *NODE_DATA(unsigned long x)
{
	return &contig_pglist_data;
}
/*
 * Debug
 */
static void R_show(struct memblock_type *type,char *s)
{
	int i;

	for(i = type->cnt - 1 ; i >= 0 ; i--)
	{
		mm_debug("Region[%d][%p - %p][%s]\n",i,
				(void *)type->regions[i].base,
				(void *)(type->regions[i].base +
					type->regions[i].size),s);
	}
}
static void B_show(unsigned long *bitmap)
{
	phys_addr_t start_pfn,end_pfn;
	unsigned long i;
	unsigned long bytes;
	unsigned char *byte_char = (unsigned char *)bitmap;

	start_pfn = phys_to_pfn(memory.start);
	end_pfn   = phys_to_pfn(memory.end);
	bytes = (end_pfn - start_pfn + 7) / 8;
	mm_debug("=============== BitMap =============\n");
	for(i = 0 ; i < bytes ; i++)
	{
		unsigned char byte = byte_char[i];
		int j;
		
		mm_debug("\nByte[%ld] ",i);
		for(j = 0 ; j < 8 ; j++)
		{
			unsigned char bit = byte & 0x01;

			mm_debug("\t%ld ",(unsigned long)bit);
			byte >>= 1;
		}
		
	}
	mm_debug("\n======================================\n");
}
/*
 * Initialize the structure of memblock.
 */
static void bootmem_init(void)
{
	PHYS_OFFSET = (unsigned long)((unsigned long)0x50000000 - 
			(unsigned long)memory_array);
	memblock.current_limit = memory.end;
	/* Initialize the regions of memory */
	memblock.memory.cnt = 1;
	memblock.memory.max = MAX_REGIONS;
	memblock.memory.regions = default_memory;
	memblock.memory.regions[0].base = memory.start;
	memblock.memory.regions[0].size = memory.end - memory.start;
	/* Initialize the regions of reserved */
	memblock.reserved.cnt = 1;
	memblock.reserved.max = MAX_REGIONS;
	memblock.reserved.regions = default_reserved;
	memblock.reserved.regions[0].base = 0;
	memblock.reserved.regions[0].size = 0;
}
/*
 * Align of memblock
 */
static phys_addr_t memblock_align_up(phys_addr_t addr,phys_addr_t align)
{
	return (addr + align - 1) & ~(align - 1);
}
static phys_addr_t memblock_align_down(phys_addr_t addr,
		phys_addr_t align)
{
	return (addr & ~(align - 1));
}
/*
 * Get bytes of bootmap
 */
static unsigned long bootmem_bootmap_bytes(unsigned long pages)
{
	return ((pages + 7) / 8);
}
/*
 * Get page of bootmap.
 */
static unsigned long bootmem_bootmap_pages(unsigned long start_pfn,
		unsigned long end_pfn)
{
	unsigned long bytes;

	bytes = bootmem_bootmap_bytes(end_pfn - start_pfn);

	return (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
}
/*
 * Check whether the two region is overlaps in address.
 */
static int memblock_overlaps_addr(phys_addr_t base1,phys_addr_t size1,
		phys_addr_t base2,phys_addr_t size2)
{
	return (base1 < (base2 + size2) && base2 < (base1 + size1));
}
/*
 * Check whether the new regions has overlap with region that has existed.
 * return 0: No overlap.
 * return 1: Overlap.
 */
static int memblock_overlaps_region(struct memblock_type *type,
		phys_addr_t base,phys_addr_t size)
{
	int i;

	for(i = type->cnt - 1 ; i >= 0 ; i--)
	{
		phys_addr_t rgbase;
		phys_addr_t rgsize;
		
		rgbase = type->regions[i].base;
		rgsize = type->regions[i].size;
		if(memblock_overlaps_addr(rgbase,rgsize,base,size) != 0)
			break;
	}
	return i == -1 ? -8 : i;
}
/*
 * Find the region for allocation.
 */
static phys_addr_t memblock_find_region(phys_addr_t start,phys_addr_t end,
		phys_addr_t size,phys_addr_t align)
{
	phys_addr_t base = end - size;
	int j;
	
	base = memblock_align_down(base,align);

	/* In case,huge size is requested */
	if(end < size)
		return MM_NOREGION;

	while(start <= base)
	{
		j = memblock_overlaps_region(&memblock.reserved,base,size);
		if(j == -8)
			return base;
		base = memblock.reserved.regions[j].base;
		if(base < size)
			break;
		base = memblock_align_down(base - size,align);
	}
	return MM_NOREGION;
}
/*
 * find the base address for region.
 */
static phys_addr_t memblock_find_base(phys_addr_t size,phys_addr_t align,
		phys_addr_t start,phys_addr_t max)
{
	struct memblock_type *type = &memblock.memory;
	int i;

	BUG_ON(size == 0);

	if(max == MEMBLOCK_ALLOC_ACCESSIBLE)
		max = memblock.current_limit;

	/*
	 * We do a top-down search,this tends to limit memory
	 * fragmentation by keeping early boot allocs near the 
	 * top of memory.
	 */
	for(i = type->cnt - 1; i >= 0 ; i--)
	{
		phys_addr_t rgnbase = type->regions[i].base;
		phys_addr_t rgnsize = type->regions[i].size;
		phys_addr_t end,found,base;

		/*
		 * The require size is bigger than current size,we
		 * search next regions.
		 */
		if(size > rgnsize)
			continue;
		/*
		 * Because the region is sorted in address that the address of
		 * next regions is bigger than the address of current regions.
		 * And we search from the biggest address of regions,if start address
		 * is bigger than address of current region that means we 
		 * can't get the bigger more than the address of current regions,
		 * and return directly.
		 */
		if((rgnbase + rgnsize) <= start)
			break;
		base = max(start,rgnbase);
		end = min(max,rgnsize + rgnbase);
	
		found = memblock_find_region(base,end,size,align);
		if(found != MM_NOREGION)
			return found;
	}
	return MM_NOREGION;
}
/*
 * Check whether the address is adjacent.
 */
static int memblock_adjacent_addr(phys_addr_t base1,phys_addr_t size1,
		phys_addr_t base2,phys_addr_t size2)
{
	if(base1 == base2 + size2)
		return 1;
	else if(base2 == base1 + size1)
		return -1;
	else
		return 0;
}
static int memblock_memory_can_coalesce(phys_addr_t addr1,
		phys_addr_t size1,phys_addr_t addr2,phys_addr_t size2)
{
	return 1;
}
static long memblock_regions_adjacent(struct memblock_type *type,
		phys_addr_t r1,phys_addr_t r2)
{
	phys_addr_t base1 = type->regions[r1].base;
	phys_addr_t size1 = type->regions[r1].size;
	phys_addr_t base2 = type->regions[r2].base;
	phys_addr_t size2 = type->regions[r2].size;

	return memblock_adjacent_addr(base1,size1,base2,size2);
}
/*
 * Remvoe a region from regions of memory or reserved.
 */
static void memblock_remove_region(struct memblock_type *type,unsigned long r)
{
	unsigned long i;

	for(i = r ; i < type->cnt - 1 ; i++)
	{
		type->regions[i].base = type->regions[i + 1].base;
		type->regions[i].size = type->regions[i + 1].size;
	}
	type->cnt--;
}
/*
 * Assumption:base addr of region 1 < base addr of regions2.
 */
static void memblock_coalesce_regions(struct memblock_type *type,
		unsigned long r1,unsigned long r2)
{
	type->regions[r1].size += type->regions[r2].size;
	memblock_remove_region(type,r2);
}
/*
 * Add a new region into Memblock.reserved
 */
static int memblock_add_region(struct memblock_type *type,
		phys_addr_t base,phys_addr_t size)
{
	int i;
	int adjacent = 0;
	unsigned long coalesced = 0;
	
	/*
	 * First add region.
	 */
	if(type->cnt == 1 && type->regions[0].size == 0)
	{
		type->regions[0].base = base;
		type->regions[0].size = size;
		return 0;
	}
	/*
	 * If the regions is adjacent,we coalesces both.
	 */
	for(i = type->cnt -1 ; i >= 0 ; i--)
	{
		phys_addr_t rgbase,rgsize;

		rgbase = type->regions[i].base;
		rgsize = type->regions[i].size;

		/*
		 * In case,memblock_find_base will get an unused address.
		 * Already have this region,so we're done.
		 */
		if((rgbase == base) && (rgsize == size))
			return 0;
		adjacent = memblock_adjacent_addr(base,size,rgbase,rgsize);
		/* Check if arch allows coalescing */
		if(adjacent != 0 && type == &memblock.memory &&
				!memblock_memory_can_coalesce(base,size,rgbase,rgsize))
			break;
		if(adjacent < 0)
		{
			type->regions[i].base -= size;
			type->regions[i].size += size;
			coalesced++;
			break;
		}
		else if(adjacent > 0)
		{
			type->regions[i].size += size;
			coalesced++;
			break;
		}

	}
	/*
	 * If we plugged a hole,we may want to also coalesce with the 
	 * next region.
	 */
	if((i < type->cnt - 1) && memblock_regions_adjacent(type,i,i+1) &&
			((type != &memblock.memory || 
			  memblock_memory_can_coalesce(type->regions[i].base,
				  type->regions[i].size,
				  type->regions[i + 1].base,
				  type->regions[i + 1].size))))
	{
		memblock_coalesce_regions(type,i,i+1);
		coalesced++;
	}
	/*
	 * Coalesed finished!
	 */
	if(coalesced)
		return coalesced;
	/*
	 * If we area out of space,we fail.It's too late to resize the array
	 * but then this shouldn't have happened in the first place.
	 */
	if(WARN_ON(type->cnt >= type->max))
		return-1;
	/*
	 * Could't coalesce the MEMBLOCK,so add it to the sorted table.
	 */
	for(i = type->cnt - 1 ; i >= 0 ; i--)
	{
		if(base < type->regions[i].base)
		{
			type->regions[i + 1].base = type->regions[i].base;
			type->regions[i + 1].size = type->regions[i].size;
		} else
		{
			type->regions[i + 1].base = base;
			type->regions[i + 1].size = size;
			break;
		}
	}
	if(base < type->regions[0].base)
	{
		type->regions[0].base = base;
		type->regions[0].size = size;
	}
	type->cnt++;
	return 0;
}
/*
 * Underlying function of memblock_alloc_bsae
 */
static phys_addr_t __memblock_alloc_base(phys_addr_t size,phys_addr_t align,
		phys_addr_t max)
{
	phys_addr_t found;

	size = memblock_align_up(size,align);
	found = memblock_find_base(size,align,0,max);
	if(found != MM_NOREGION &&
		!memblock_add_region(&memblock.reserved,found,size))
		return found;
	return 0;
}
/*
 * Allocate a new region to bitmap.
 */
static phys_addr_t memblock_alloc_base(phys_addr_t size,phys_addr_t align,
		phys_addr_t max)
{
	phys_addr_t alloc;

	alloc = __memblock_alloc_base(size,align,max);
	if(alloc == 0)
		panic("PANIC:Can't find address\n ");
	return alloc;
}
/*
 * Init bootmem in core
 */
static void init_bootmem_core(struct bootmem_data *bdata,unsigned long bitmap,
		unsigned long start_pfn,unsigned long end_pfn)
{
	unsigned long mapsize;

	bdata->node_min_pfn = start_pfn;
	bdata->node_low_pfn = end_pfn;
	bdata->node_bootmem_map = (void *)phys_to_virt(bitmap);

	mapsize = bootmem_bootmap_bytes(end_pfn - start_pfn);
	
	memset(bdata->node_bootmem_map,0xFF,mapsize);
}
/*
 * Init bootmem in node
 */
static void init_bootmem_node(struct pglist_data *pgdat,unsigned long bitmap,
		unsigned long start_pfn,unsigned long end_pfn)
{
	init_bootmem_core(&pgdat->bdata,bitmap,start_pfn,end_pfn);
}
/*
 * free bootmem regions.
 */
static void __free(struct bootmem_data *bdata,unsigned long start,
		unsigned long end)
{
	unsigned long *bitmap = (unsigned long *)bdata->node_bootmem_map;
	unsigned long sidx,eidx;
	unsigned long idx;

	sidx = start - bdata->node_min_pfn;
	eidx = end - bdata->node_min_pfn;
	for(idx = 0 ; idx < eidx ; idx++)
	{
		test_and_clear_bit(idx,bitmap);
	}
}
/*
 * reserve bootmem regions.
 */
static void __reserve(struct bootmem_data *bdata,unsigned long start,
		unsigned long end,unsigned long flags)
{
	unsigned long *bitmap = (unsigned long *)bdata->node_bootmem_map;
	unsigned long sidx,eidx;
	unsigned long idx;

	sidx = start - bdata->node_min_pfn;
	eidx = end - bdata->node_min_pfn;

	mm_debug("sidx %p eidx %p\n",(void *)sidx,(void *)eidx);
	for(idx = sidx ; idx < eidx ; idx++)
	{
		test_and_set_bit(idx,bitmap);
	}
}
/*
 * Mark memblock
 */
static void mark_bootmem(struct bootmem_data *bdata,
		struct memblock_region *reg,unsigned long reserve,unsigned long flags)
{
	unsigned long start_pfn,end_pfn;

	start_pfn = phys_to_pfn(reg->base);
	end_pfn = phys_to_pfn(reg->base + reg->size);

	mm_debug("Check [%p - %p]\n",(void *)start_pfn,(void *)end_pfn);

	if(reserve)
		__reserve(bdata,start_pfn,end_pfn,flags);
	else
		__free(bdata,start_pfn,end_pfn);
}
/*
 * Free bit
 */
static void free_bootmem(struct bootmem_data *bdata,
		struct memblock_region *reg)
{
	mark_bootmem(bdata,reg,0,0);
}
/*
 * Reserved bit
 */
static void reserve_bootmem(struct bootmem_data *bdata,
		struct memblock_region *reg)
{
	mark_bootmem(bdata,reg,1,0);
}
/*
 * Initialize the arm bootmem
 */
static void arm_bootmem_init(unsigned long start_pfn,
		unsigned long end_pfn)
{
	phys_addr_t bitmap;
	struct memblock_region *reg;
	unsigned int pages_alloc;
	struct pglist_data *pgdat;
	int i;

	pages_alloc = bootmem_bootmap_pages(start_pfn,end_pfn);
	bitmap = memblock_alloc_base(pages_alloc << PAGE_SHIFT,L1_CACHE_BYTES,
			pfn_to_phys(end_pfn));
	
	pgdat = NODE_DATA(0);
	init_bootmem_node(pgdat,bitmap,start_pfn,end_pfn);

	/*
	 * Clear bit that contain in useful physical memory.
	 */
	for_each_memblock(memblock.memory,reg)
	{
		free_bootmem(&pgdat->bdata,reg);
	}
	/*
	 * Set bit that reserved.
	 */
	for_each_memblock(memblock.reserved,reg)
	{
		reserve_bootmem(&pgdat->bdata,reg);	
	}

	R_show(&memblock.reserved,"Final show");
	B_show(pgdat->bdata.node_bootmem_map);
}



/*
 * Arch init,top level
 */
void arch_init(void)
{
	
	bootmem_init();
	arm_bootmem_init(phys_to_pfn(memory.start),phys_to_pfn(memory.end));
}
