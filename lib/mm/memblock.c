#include "../../include/linux/memblock.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/page.h"
#include "../../include/linux/kernel.h"
#include "../../include/linux/types.h"
#include "../../include/linux/list.h"
#include "../../include/linux/mmzone.h"
#include "../../include/linux/memory.h"
#include "../../include/linux/highmem.h"
#include "../../include/linux/mm_type.h"
#include "../../include/linux/error.h"
#include "../../include/linux/nodemask.h"
#include <malloc.h>
#include <string.h>

struct memblock_region default_memory[INIT_MEMBLOCK_REGIONS + 1];
struct memblock_region default_reserved[INIT_MEMBLOCK_REGIONS + 1];
struct memblock memblock = {
	.current_limit = MAX_REGIONS,
};

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
static unsigned long bootmap_bytes(unsigned long pages)
{
	return ((pages + 7) / 8);
}
/*
 * Get page of bootmap.
 */
unsigned long bootmem_bootmap_pages(unsigned long start_pfn,
		unsigned long end_pfn)
{
	unsigned long bytes;

	bytes = bootmap_bytes(end_pfn - start_pfn);

	return (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
}
/*
 * Check whether the two region is overlaps in address.
 */
static int memblock_overlaps_addr(phys_addr_t base1,phys_addr_t size1,
		phys_addr_t base2,phys_addr_t size2)
{
	return ((base1 < (base2 + size2)) && (base2 < (base1 + size1)));
}
/*
 * Check whether the new regions has overlap with region that has existed.
 * return 0: No overlap.
 * return 1: Overlap.
 */
static int memblock_overlaps_region(struct memblock_type *type,
		phys_addr_t base,phys_addr_t size)
{
	unsigned long i;

	for(i = 0; i < type->cnt ; i++)
	{
		phys_addr_t rgbase = type->regions[i].base;
		phys_addr_t rgsize = type->regions[i].size;
		if(memblock_overlaps_addr(rgbase,rgsize,base,size))
			break;
	}
	return (i < type->cnt) ? i : -1;
}
/*
 * Find the region for allocation.
 */
static phys_addr_t memblock_find_region(phys_addr_t start,phys_addr_t end,
		phys_addr_t size,phys_addr_t align)
{
	phys_addr_t base,res_base;
	int j;
	
	/* In case,huge size is requested */
	if(end < size)
		return MEMBLOCK_ERROR;

	base = memblock_align_down((end - size),align);
	/*
	 * Prevent allocateions returning 0 as it's also used to
	 * indicate an allocation failure.
	 */
	if(start == 0)
		start = PAGE_SIZE;

	while(start <= base)
	{
		j = memblock_overlaps_region(&memblock.reserved,base,size);
		if(j < 0)
			return base;
		res_base = memblock.reserved.regions[j].base;
		if(res_base < size)
			break;
		base = memblock_align_down(res_base - size,align);
	}
	return MEMBLOCK_ERROR;
}
/*
 * find the base address for region.
 */
static phys_addr_t __init_memblock memblock_find_base(phys_addr_t size,
		phys_addr_t align,phys_addr_t start,phys_addr_t end)
{
	long i;

	BUG_ON(0 == size);

	/* Pump up max_addr */
	if(end == MEMBLOCK_ALLOC_ACCESSIBLE)
		end = memblock.current_limit;
	
	/*
	 * We do a top-down search,this tends to limit memory
	 * fragmentation by keeping early boot allocs near the 
	 * top of memory.
	 */
	for(i = memblock.memory.cnt - 1; i >= 0 ; i--)
	{
		phys_addr_t memblockbase = memblock.memory.regions[i].base;
		phys_addr_t memblocksize = memblock.memory.regions[i].size;
		phys_addr_t bottom,top,found;
		
		if(memblocksize < size)
			continue;
		
		if((memblockbase + memblocksize) <= start)
			break;
		bottom = max(memblockbase,start);
		top = min(memblockbase + memblocksize,end);
		if(bottom >= top)
			continue;

		found = memblock_find_region(bottom,top,size,align);
		if(found != MEMBLOCK_ERROR)
			return found;
	}
	return MEMBLOCK_ERROR;
}
/*
 * Check whether the address is adjacent.
 */
static long __init_memblock memblock_adjacent_addr(phys_addr_t base1,
		phys_addr_t size1,phys_addr_t base2,phys_addr_t size2)
{
	if(base2 == base1 + size1)
		return 1;
	else if(base1 == base2 + size2)
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
static void __init_memblock memblock_remove_region(struct memblock_type *type,
						unsigned long r)
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
	unsigned long coalesced = 0;
	long adjacent,i;
	
	if((type->cnt == 1) && (type->regions[0].size == 0))
	{
		type->regions[0].base = base;
		type->regions[0].size = size;
		return 0;
	}
	/*
	 * First try and coalesce this MEMBLOCK with another.
	 */
	for(i = 0 ; i < type->cnt ; i++)
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
		if(adjacent > 0)
		{
			type->regions[i].base -= size;
			type->regions[i].size += size;
			coalesced++;
			break;
		}
		else if(adjacent < 0)
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
		return -1;
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
phys_addr_t __init __memblock_alloc_base(phys_addr_t size,phys_addr_t align,
		phys_addr_t max)
{
	phys_addr_t found;

	/*
	 * We align the size to limit fragmentation.Without this.a lot of
	 * small allocs quickly eat up the whole reserved array on sparc.
	 */
	size = memblock_align_up(size,align);

	found = memblock_find_base(size,align,0,max);
	if(found != MEMBLOCK_ERROR &&
		memblock_add_region(&memblock.reserved,found,size) >= 0)
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
		panic("ERROR:Failed to allocate %u bytes below %u\n",
				size,max);
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
static unsigned long __init init_bootmem_core(struct bootmem_data *bdata,
		unsigned long mapstart,unsigned long start,unsigned long end)
{
	unsigned long mapsize;

	bdata->node_min_pfn = start;
	bdata->node_low_pfn = end;
	bdata->node_bootmem_map = phys_to_mem(mapstart);
	link_bootmem(bdata);

	mapsize = bootmap_bytes(end - start);
	memset(bdata->node_bootmem_map,0xFF,mapsize);

	bdebug("nid=%d start=%p map=%p end=%p mapsize=%p\n",
			(unsigned int)(bdata - bootmem_node_data),
			(void *)start,(void *)mapstart,
			(void *)end,(void *)mapsize);

	return mapsize;
}
/*
 * Init bootmem in node
 */
static long __init init_bootmem_node(struct pglist_data *pgdat,
		unsigned long bitmap,unsigned long start_pfn,unsigned long end_pfn)
{
	return init_bootmem_core(pgdat->bdata,bitmap,start_pfn,end_pfn);
}
/*
 * Initialize the arm bootmem
 */
void arm_bootmem_init(unsigned int start_pfn,
		unsigned int end_pfn)
{
	struct memblock_region *reg;
	unsigned int boot_pages;
	phys_addr_t bitmap;
	pg_data_t *pgdat;

	/*
	 * Allocate the bootmem bitmap page.This must be in a region
	 * of memory which has already mapped.
	 */
	boot_pages = bootmem_bootmap_pages(start_pfn,end_pfn);
	bitmap = memblock_alloc_base(boot_pages << PAGE_SHIFT,L1_CACHE_BYTES,
			__pfn_to_phys(end_pfn));
	
	/*
	 * Initialise the bootmem allocator,handing the 
	 * memory banks over to bootmem.
	 */
	node_set_online(0);
	pgdat = NODE_DATA(0);
	init_bootmem_node(pgdat,bitmap,start_pfn,end_pfn);

	/*
	 * Free the lowmem regions from memblock into bootmem.
	 */
	for_each_memblock(memory,reg)
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
	for_each_memblock(reserved,reg)
	{
		unsigned long start = memblock_region_reserved_base_pfn(reg);
		unsigned long end = memblock_region_reserved_end_pfn(reg);
		
		if(end > end_pfn)
			end = end_pfn;
		if(start > end)
			break;
		
		reserve_bootmem(pfn_to_phys(start),(end - start) << PAGE_SHIFT,
				BOOTMEM_DEFAULT);	
	}
}
/*
 * Calculate the pages of pglist_data
 */
static void calculate_node_totalpages(struct pglist_data *pgdat,
		unsigned long *zone_sizes,unsigned long *zhole_size)
{
	unsigned long realpages,totalpages = 0;
	int i;

	for(i = 0 ; i < MAX_NR_ZONES ; i++)
	{
		totalpages += zone_spanned_pages_in_node(pgdat->node_id,
				i,zone_sizes);
	}
	realpages = totalpages;
	for(i = 0 ; i < MAX_NR_ZONES ; i++)
	{
		realpages -= zone_absent_pages_in_node(pgdat->node_id,
				i,zhole_size);
	}
	pgdat->node_spanned_pages = totalpages;
	pgdat->node_present_pages = realpages;
}
/*
 * Alloc remap in bootmem.
 */
static inline void *alloc_remap(int nid,unsigned long size)
{
	return NULL;
}
/*
 * Allocate node mem map.
 */
static void alloc_node_mem_map(struct pglist_data *pgdat)
{
	/*
	 * Skip empty nodes.
	 */
	if(!pgdat->node_spanned_pages)
		return;
#ifdef CONFIG_FLAT_NODE_MEM_MAP
	if(!pgdat->node_mem_map)
	{
		unsigned long size,start,end;
		struct page *map;

		/*
		 * The zone's endpoints aren't required to be MAX_ORDER
		 * aligned but the node_mem_map endpoints must be in order
		 * for the buddy allocator to function correctly.
		 */
		start = pgdat->node_start_pfn & ~(MAX_ORDER_NR_PAGES - 1);
		end   = pgdat->node_start_pfn + pgdat->node_spanned_pages;
		end   = ALIGN(end,MAX_ORDER_NR_PAGES);
		size  = (end - start) * sizeof(struct page);
		map   = alloc_remap(pgdat->node_id,size);
		if(!map)
			map = (struct page *)(unsigned long)alloc_bootmem_node(pgdat,
					size >> PAGE_SHIFT);
		/*
		 * In order to use node_mem_map directly,we use virtual memory address 
		 * to replace the physcial address.Note,all address which allocate from
		 * virtual memory use virtual memory address.
		 */
		pgdat->node_mem_map =  (struct page *)map + 
				(pgdat->node_start_pfn - start);
	}
#ifndef CONFIG_NEED_MULTIPLE_NODES
	if(pgdat == NODE_DATA(0))
	{
		mem_map = (struct page *)(unsigned long)mem_to_phys(NODE_DATA(0)->node_mem_map);
	}
#endif
#endif
}
/*
 * Init area
 * Arguement:start_pfn , zone_sizes and zhold_size in PFN.
 */
static void free_area_init_node(int nid,unsigned long *zone_sizes,
		unsigned long start_pfn,unsigned long *zhole_size)
{
	struct pglist_data *pgdat = NODE_DATA(nid);

	pgdat->node_id = nid;
	pgdat->node_start_pfn = start_pfn;
	calculate_node_totalpages(pgdat,zone_sizes,zhole_size);

	alloc_node_mem_map(pgdat);
#ifdef CONFIG_FLAT_NODE_MEM_MAP
	mm_debug("free_area_init_node:node %lu,pgdat %p,node_mem_map %p\n",
			pgdat->node_id,(void *)pgdat,(void *)pgdat->node_mem_map);
#endif
	free_area_init_core(pgdat,zone_sizes,zhole_size);
}
/*
 * ARM bootmem free
 * min,max_low and max_high in PFN.
 */
void arm_bootmem_free(unsigned long min,unsigned long max_low,
		unsigned long max_high)
{
	unsigned long zone_sizes[MAX_NR_ZONES],zhole_size[MAX_NR_ZONES];
	struct memblock_region *reg;

	/*
	 * Initialise the zones.
	 */
	memset(zone_sizes,0,sizeof(zone_sizes));

	/*
	 * The memory size has already been determmined.If we need
	 * to do anything fancy with the allocation of this memory
	 * to the zones,now is the time to do it.
	 */
	zone_sizes[0] = max_low - min;
#ifdef CONFIG_HIGHMEM
	zone_sizes[ZONE_HIGHMEM] = max_high - max_low;
#endif
	/*
	 * Calculate the size of the holes.
	 * holes = node_size - sum(bank_size).
	 */
	memcpy(zhole_size,zone_sizes,sizeof(zone_sizes));
	for_each_memblock(memory,reg)
	{
		unsigned long start = memblock_region_memory_base_pfn(reg);
		unsigned long end   = memblock_region_memory_end_pfn(reg);

		if(start < max_low)
		{
			unsigned long low_end = min(end,max_low);
			zhole_size[0] -= low_end - start;
		}
#ifdef CONFIG_HIGHMEM
		if(end > max_low)
		{
			unsigned long high_start = max(start,max_low);
			zhole_size[1] -= end - high_start;
		}
#endif
	}
	free_area_init_node(0,zone_sizes,min,zhole_size);
}
