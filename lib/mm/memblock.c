#include "../../include/linux/memblock.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/page.h"
#include "../../include/linux/kernel.h"
#include "../../include/linux/types.h"
#include "../../include/linux/list.h"
#include <malloc.h>
#include <string.h>

struct memblock_region default_memory[INIT_MEMBLOCK_REGIONS + 1];
struct memblock_region default_reserved[INIT_MEMBLOCK_REGIONS + 1];
struct memblock memblock = {
	.current_limit = MAX_REGIONS,
};


struct page *mem_map;
struct list_head bdata_list;

struct pglist_data contig_pglist_data;

/*
 * memory address to phys address
 */
phys_addr_t mem_to_phys(unsigned int addr)
{
	return 0;
}
/*
 * pfn_to_mem
 */
void *pfn_to_mem(unsigned int idx)
{
	unsigned int *ret;

	idx = pfn_to_phys(idx);
	ret = phys_to_mem(idx);

	return (void *)ret;
}
static struct pglist_data *NODE_DATA(unsigned long x)
{
	return &contig_pglist_data;
}
/*
 * Initialize the structure of memblock.
 */
void __init memblock_init(void)
{
	/* Initialize the regions of memory */
	memblock.memory.cnt = 1;
	memblock.memory.max = INIT_MEMBLOCK_REGIONS;
	memblock.memory.regions = default_memory;
	memblock.memory.regions[0].base = 0;
	memblock.memory.regions[0].size = 0;
	/* Initialize the regions of reserved */
	memblock.reserved.cnt = 1;
	memblock.reserved.max = INIT_MEMBLOCK_REGIONS;
	memblock.reserved.regions = default_reserved;
	memblock.reserved.regions[0].base = 0;
	memblock.reserved.regions[0].size = 0;

	memblock.memory.regions[INIT_MEMBLOCK_REGIONS].base 
						= (phys_addr_t)RED_INACTIVE;
	memblock.reserved.regions[INIT_MEMBLOCK_REGIONS].base
						= (phys_addr_t)RED_INACTIVE;

	memblock.current_limit = MEMBLOCK_ALLOC_ANYWHERE;
	/* Init list of bdata */
	INIT_LIST_HEAD(&bdata_list);
}
/*
 * Set current limit of physical address of memblock.
 */
void memblock_set_current_limit(phys_addr_t limit)
{
	memblock.current_limit = limit;
}
/*
 * Dump the information of memory and reserved.
 */
static void __init_memblock memblock_dump(struct memblock_type *type,
		char *name)
{
	int i;

	for(i = 0 ; i < type->cnt ; i++)
	{
		mm_debug("Memblock.%s.regions[%d]:Base %08x End %08x Size %08x\n",
				name,i,(unsigned int)type->regions[i].base,
				(unsigned int)(type->regions[i].base + type->regions[i].size),
				(unsigned int)type->regions[i].size);
	}
}
/*
 * Dump all memblock.type
 */
void __init_memblock memblock_dump_all(void)
{
	memblock_dump(&memblock.memory,"memory");
	memblock_dump(&memblock.reserved,"reserved");
}
/*
 * Analyze and update the total size of memblock.memory.
 */
void __init memblock_analyze(void)
{
	int i;

	memblock.memory_size = 0;

	for(i = 0 ; i < memblock.memory.cnt ; i++)
		memblock.memory_size += memblock.memory.regions[i].size;
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
		return 1;
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
		return 0;
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
	return 1;
}
/*
 * Add memblock region that from membank to memblock.memory.
 */
long __init_memblock memblock_add(phys_addr_t base,phys_addr_t size)
{
	return memblock_add_region(&memblock.memory,base,size);
}
/*
 * Add a region of reserve in top APT
 */
int memblock_reserve(phys_addr_t base,phys_addr_t size)
{
	struct memblock_type *reg = &memblock.reserved;

	return memblock_add_region(reg,base,size);
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
		memblock_add_region(&memblock.reserved,found,size))
		return found;
	mm_err("can't alloc size %08x\n",size);
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
 * memblock_alloc
 */
phys_addr_t memblock_alloc(phys_addr_t size,phys_addr_t align)
{
	return memblock_alloc_base(size,align,MEMBLOCK_ALLOC_ACCESSIBLE);
}
static void link_bootmem(struct bootmem_data *bdata)
{
	struct list_head *iter;

	list_for_each(iter,&bdata_list)
	{
		struct bootmem_data *bd;

		bd = list_entry(iter,struct bootmem_data,list);

		if(bdata->node_min_pfn < bd->node_min_pfn)
			break;
	}
	list_add(&bdata->list,&bdata_list);
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
	bdata->node_bootmem_map = phys_to_mem(bitmap);

	link_bootmem(bdata);

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
static void __free(struct bootmem_data *bdata,unsigned long sidx,
		unsigned long eidx)
{
	unsigned long *bitmap = (unsigned long *)bdata->node_bootmem_map;
	unsigned long idx;

	for(idx = 0 ; idx < eidx ; idx++)
	{
		test_and_clear_bit(idx,bitmap);
	}
}
/*
 * reserve bootmem regions.
 */
static int __reserve(struct bootmem_data *bdata,unsigned long sidx,
		unsigned long eidx,unsigned long flags)
{
	unsigned long *bitmap = (unsigned long *)bdata->node_bootmem_map;
	unsigned long idx;
	unsigned long set_flags = flags & 0x01;

	for(idx = sidx ; idx < eidx ; idx++)
	{
		if(test_and_set_bit(idx,bitmap))
		{
			if(set_flags)
			{
				__free(bdata,sidx,idx);
				return MM_NOREGION;
			}
		}
	}
	return 0;
}
/*
 * Mark bootmem node
 */
static int mark_bootmem_node(struct bootmem_data *bdata,
		unsigned long start,unsigned long end,
		unsigned long reserve,unsigned long flags)
{
	unsigned long sidx,eidx;

	sidx = start - bdata->node_min_pfn;
	eidx = end - bdata->node_min_pfn;

	if(reserve)
		return __reserve(bdata,sidx,eidx,flags);
	else
		__free(bdata,sidx,eidx);
	return 0;
}
/*
 * Mark memblock
 */
static int mark_bootmem(unsigned long start,unsigned long end,
		unsigned long reserve,unsigned long flags)
{
	unsigned long pos;
	struct bootmem_data *bdata;

	pos = start;
	list_for_each_entry(bdata,&bdata_list,list)
	{
		int err;
		unsigned long max;

		if(pos < bdata->node_min_pfn ||
				pos >= bdata->node_low_pfn)
			continue;
		
		max = min(end,bdata->node_low_pfn);
		err = mark_bootmem_node(bdata,start,end,reserve,flags);
		if(err && reserve)
		{
			mark_bootmem(start,end,0,0);
			return err;
		}
		if(max == end)
			return 0;
		pos = bdata->node_min_pfn;
	}
	mm_err("%s err\n",__FUNCTION__);
}
/*
 * Free bit
 */
static void free_bootmem(phys_addr_t addr,phys_addr_t size)
{
	unsigned long start,end;

	start = PFN_UP(addr);
	end = PFN_DOWN(addr + size);

	mark_bootmem(start,end,0,0);
}
/*
 * Reserved bit
 */
static void reserve_bootmem(phys_addr_t addr,phys_addr_t size)
{
	unsigned long start,end;

	start = PFN_UP(addr);
	end   = PFN_DOWN(addr + size);
	/* Note for debug and we set flags as 1.*/
	mark_bootmem(start,end,1,1);
}
/*
 * The start pfn of memory region.
 */
static unsigned long memblock_region_memory_base_pfn(struct memblock_region *reg)
{
	return PFN_UP(reg->base);
}
/*
 * The end pfn of memory region.
 */
static unsigned long memblock_region_memory_end_pfn(struct memblock_region *reg)
{
	return PFN_DOWN(reg->base + reg->size);
}
/*
 * The start pfn of reserved region
 */
static unsigned long memblock_region_reserved_base_pfn(struct memblock_region *reg)
{
	return PFN_UP(reg->base);
}
/*
 * The end pfn of reserved region.
 */
static unsigned long memblock_region_reserved_end_pfn(struct memblock_region *reg)
{
	return PFN_DOWN(reg->base + reg->size);
}
/*
 * Initialize the arm bootmem
 */
void arm_bootmem_init(unsigned int start_pfn,
		unsigned int end_pfn)
{
	phys_addr_t bitmap;
	struct memblock_region *reg;
	unsigned int pages_alloc;
	struct pglist_data *pgdat;
	int i;
	struct bootmem_data *bd;

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
		unsigned long start = memblock_region_memory_base_pfn(reg);
		unsigned long end   = memblock_region_memory_end_pfn(reg);

		if(end > end_pfn)
			end = end_pfn;
		if(start > end)
			break;

		free_bootmem(pfn_to_phys(start),(end - start) << PAGE_SHIFT);
	}
	/*
	 * Set bit that reserved.
	 */
	for_each_memblock(memblock.reserved,reg)
	{
		unsigned long start = memblock_region_reserved_base_pfn(reg);
		unsigned long end = memblock_region_reserved_end_pfn(reg);
		
		if(end > end_pfn)
			end = end_pfn;
		if(start > end)
			break;
		
		reserve_bootmem(pfn_to_phys(start),(end - start) << PAGE_SHIFT);	
	}
}
/*
 * Calculate spanned pages
 */
static unsigned long zone_spanned_pages_in_node(int nid,
		unsigned long type,unsigned long *zone_sizes)
{
	return zone_sizes[type];
}
/*
 * Calculate absent pages.
 */
static unsigned long zone_absent_pages_in_node(int nid,
		unsigned long type,unsigned long *zhole_size)
{
	return zhole_size[type];
}
/*
 * Calculate the pages of pglist_data
 */
static void calculate_node_pages(struct pglist_data *pgdat,
		unsigned long *zone_sizes,unsigned long *zhole_size)
{
	unsigned long realpages,totalpages = 0;
	int i;

	for(i = 0 ; i < MAX_REGIONS ; i++)
	{
		totalpages += zone_spanned_pages_in_node(pgdat->node_id,
				i,zone_sizes);
	}
	realpages = totalpages;
	for(i = 0 ; i < MAX_REGIONS ; i++)
	{
		realpages -= zone_absent_pages_in_node(pgdat->node_id,
				i,zhole_size);
	}
	pgdat->node_spanned_pages = totalpages;
	pgdat->node_present_pages = realpages;
}
/*
 * Alloc bootmem_core
 */
static unsigned int *alloc_bootmem_core(struct bootmem_data *bdata,
		unsigned long size)
{
	unsigned long sidx = 0;
	long idx;
	unsigned long eidx;
	void *region;

Again:
	sidx = find_next_zero_bit(bdata->node_bootmem_map,size,sidx);
	eidx = sidx + size;
	for(idx = sidx ; idx < eidx ; idx++)
	{
		if(test_bit(idx,bdata->node_bootmem_map))
		{
			sidx++;
			goto Again;
		}
	}
	if(__reserve(bdata,sidx,eidx,0) != MM_NOREGION)
	{
		region = pfn_to_mem(sidx);
		return region;
	}
	return 0;
}
/*
 * Init area
 */
static void free_area_init_node(unsigned long *zone_sizes,
		unsigned long start_pfn,unsigned long *zhole_size)
{
	struct pglist_data *pgdat = NODE_DATA(0);
	unsigned long size;

	pgdat->node_id = 0;
	pgdat->node_start_pfn = start_pfn;

	calculate_node_pages(pgdat,zone_sizes,zhole_size);

	size = pgdat->node_spanned_pages * sizeof(struct page);

	pgdat->node_mem_map = alloc_bootmem_core(&pgdat->bdata,size >> PAGE_SHIFT);

	mem_map = pgdat->node_mem_map;
}
/*
 * ARM bootmem free
 */
static void arm_bootmem_free(unsigned long min,unsigned long max_low,
		unsigned long max_high)
{
	unsigned long zone_sizes[MAX_REGIONS],zhole_size[MAX_REGIONS];
	struct memblock_region *reg;

	memset(zone_sizes,0,sizeof(zone_sizes));

	zone_sizes[0] = max_low - min;
#ifdef CONFIG_HIGHMEM
	zone_sizes[1] = max_high - max_low;
#endif
	memcpy(zhole_size,zone_sizes,sizeof(zone_sizes));

	for_each_memblock(memblock.memory,reg)
	{
		unsigned long start = memblock_region_memory_base_pfn(reg);
		unsigned long end   = memblock_region_memory_end_pfn(reg);

		zhole_size[0] -= end - start;
#ifdef CONFIG_HIGHMEM
		zhole_size[1] -= max_high - end;
#endif
	}
	free_area_init_node(zone_sizes,min,zhole_size);
}

/*
 * Arch init,top level
 */
void arch_init(void)
{
	struct pglist_data *pgdat = NODE_DATA(0);
#if 0
	arm_bootmem_init(phys_to_pfn(memory.start),phys_to_pfn(memory.end));
	arm_bootmem_free(phys_to_pfn(memory.start),phys_to_pfn(memory.end),
			phys_to_pfn(memory.end));
	B_show(pgdat->bdata.node_bootmem_map);
#endif
}
