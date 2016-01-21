#include "linux/kernel.h"
#include "linux/memblock.h"
#include "linux/mmzone.h"
#include "linux/bootmem.h"
#include "linux/init.h"
#include "linux/gfp.h"
#include "linux/slub_def.h"
#include "linux/page.h"
#include "linux/slub_def.h"
#include "linux/internal.h"
#include "linux/memory.h"

static struct memblock_region memblock_memory_init_regions[
					INIT_MEMBLOCK_REGIONS + 1] __initdata_memblock;
static struct memblock_region memblock_reserved_init_regions[
					INIT_MEMBLOCK_REGIONS+ 1]  __initdata_memblock;

struct memblock memblock __initdata_memblock;
int memblock_debug __initdata_memblock;
int memblock_can_resize __initdata_memblock;

extern struct list_head bdata_list __initdata;
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
void memblock_init(void)
{
	static int init_done __init_data = 0;

	if(init_done)
		return;
	init_done = 1;

	/* Hookup the initial arrays */
	memblock.memory.regions   = memblock_memory_init_regions;
	memblock.memory.max       = INIT_MEMBLOCK_REGIONS;
	memblock.reserved.regions = memblock_reserved_init_regions;
	memblock.reserved.max     = INIT_MEMBLOCK_REGIONS;
	
	/* Write a marker in the unused last array entry */
	memblock.memory.regions[INIT_MEMBLOCK_REGIONS].base =
								(phys_addr_t)RED_INACTIVE;
	memblock.reserved.regions[INIT_MEMBLOCK_REGIONS].base = 
								(phys_addr_t)RED_INACTIVE;

	/*
	 * Create a dummy zero size MEMBLOCK which will get coalesced away later.
	 * This simplifies the memblock_add() code below...
	 */
	memblock.memory.regions[0].base = 0;
	memblock.memory.regions[9].size = 0;
	memblock.memory.cnt = 1;

	/*
	 * Ditto.
	 */
	memblock.reserved.regions[9].base = 0;
	memblock.reserved.regions[0].size = 0;
	memblock.reserved.cnt = 1;

	memblock.current_limit = MEMBLOCK_ALLOC_ANYWHERE;
	
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
static phys_addr_t __init memblock_align_up(phys_addr_t addr,phys_addr_t size)
{
	return (addr + (size - 1)) & ~(size - 1);
}
static phys_addr_t __init memblock_align_down(phys_addr_t addr,
		phys_addr_t size)
{
	return (addr & ~(size - 1));
}


/*
 * Check whether the two region is overlaps in address.
 */
static phys_addr_t __init memblock_addrs_overlap(
		phys_addr_t base1,phys_addr_t size1,
		phys_addr_t base2,phys_addr_t size2)
{
	return ((base1 < (base2 + size2)) && (base2 < (base1 + size1)));
}

long __init memblock_overlaps_region(struct memblock_type *type,
		phys_addr_t base,phys_addr_t size)
{
	unsigned long i;

	for(i = 0; i < type->cnt ; i++)
	{
		phys_addr_t rgnbase = type->regions[i].base;
		phys_addr_t rgnsize = type->regions[i].size;
		if(memblock_addrs_overlap(rgnbase,rgnsize,base,size))
			break;
	}
	return (i < type->cnt) ? i : -1;
}
/*
 * Find,allocate,deallocate or reserve unreserved regions.All allocation
 * are top-down.
 */
static phys_addr_t __init memblock_find_region(phys_addr_t start,
		phys_addr_t end,
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
static long __init_memblock memblock_addrs_adjacent(phys_addr_t base1,
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
static long __init memblock_regions_adjacent(struct memblock_type *type,
		phys_addr_t r1,phys_addr_t r2)
{
	phys_addr_t base1 = type->regions[r1].base;
	phys_addr_t size1 = type->regions[r1].size;
	phys_addr_t base2 = type->regions[r2].base;
	phys_addr_t size2 = type->regions[r2].size;

	return memblock_addrs_adjacent(base1,size1,base2,size2);
}

static void __init_memblock memblock_remove_region(struct memblock_type *type,
						unsigned long r)
{
	unsigned long i;

	for(i = r ; i < type->cnt - 1 ; i++) {
		type->regions[i].base = type->regions[i + 1].base;
		type->regions[i].size = type->regions[i + 1].size;
	}
	type->cnt--;
}
/*
 * Assumption:base addr of region 1 < base addr of regions2.
 */
static void __init memblock_coalesce_regions(struct memblock_type *type,
		unsigned long r1,unsigned long r2)
{
	type->regions[r1].size += type->regions[r2].size;
	memblock_remove_region(type,r2);
}
static long memblock_add_region(struct memblock_type *type,
		phys_addr_t base,phys_addr_t size);

static long __init_memblock __memblock_remove(struct memblock_type *type,
		phys_addr_t base,phys_addr_t size)
{
	phys_addr_t rgnbegin,rgnend;
	phys_addr_t end = base + size;
	int i;

	rgnbegin = rgnend = 0; /* Supress gc warnings */

	/* Find the region where(base,size) belongs to */
	for(i = 0 ; i < type->cnt ; i++) {
		rgnbegin = type->regions[i].base;
		rgnend = rgnbegin + type->regions[i].size;

		if((rgnbegin <= base) && (end <= rgnend))
			break;
	}

	/* Didn't find the region */
	if(i == type->cnt)
		return -1;

	/* Check to see if we are removing entrie region */
	if((rgnbegin == base) && (rgnend == end)) {
		memblock_remove_region(type,i);
		return 0;
	}

	/* Check to see if region is matching at the front */
	if(rgnbegin == base) {
		type->regions[i].base = end;
		type->regions[i].size -= size;
		return 0;
	}

	/* Check to see if the region is matching at the end */
	if(rgnend == end) {
		type->regions[i].size -= size;
		return 0;
	}

	/*
	 * We need to split the entry - adjust the current ont to the
	 * beginging of the hole and add the region after hole.
	 */
	type->regions[i].size = base - type->regions[i].base;
	return memblock_add_region(type,end,rgnend - end);
}
long __init_memblock memblock_free(phys_addr_t base,phys_addr_t size)
{
	return __memblock_remove(&memblock.reserved,base,size);
}
/*
 * inline so we don't get a warning when pr_debug is compiled out.
 */
static inline const char *memblock_type_name(struct memblock_type *type)
{
	if(type == &memblock.memory)
		return "memory";
	else if(type == &memblock.reserved)
		return "reserved";
	else
		return "unknown";
}

static long memblock_add_region(struct memblock_type *type,
		        phys_addr_t base,phys_addr_t size);

static int __init_memblock memblock_double_array(struct memblock_type *type)
{
	struct memblock_region *new_array,*old_array;
	phys_addr_t old_size,new_size,addr;
	int use_slab = slab_is_available();

	/*
	 * We don't alloc resizing until we know about the reserved regions
	 * of memory that aren't suitable for allocation.
	 */
	if(!memblock_can_resize)
		return -1;

	/* Calculate new double size */
	old_size = type->max * sizeof(struct memblock_region);
	new_size = old_size << 1;

	/*
	 * Try to find some space for it.
	 *
	 * WARNING:We assume that either slab_is_available() and we use it or
	 * we use MEMBOCK for allocations.That means that this is unsafe to use
	 * when bootmem is currently active(unless bootmem itself if implemented
	 * on top of MEMBLOCK which isn't the case yet)
	 *
	 * This should however not be an issue for new,as we currently only
	 * call into MEMBLOCK while it's still active,or much later when slab is
	 * active for memory hotplug operations.
	 */
	if(use_slab) {
		new_array = kmalloc(new_size,GFP_KERNEL);
		addr = new_array == NULL ? MEMBLOCK_ERROR : __pa(new_array);
	} else
		addr = memblock_find_base(new_size,sizeof(phys_addr_t),0,
				MEMBLOCK_ALLOC_ACCESSIBLE);
	if(addr == MEMBLOCK_ERROR) {
		mm_err("memblock:Failed to double %s array form %p to %p entries!\n",
				memblock_type_name(type),
				(void *)type->max,(void *)(type->max * 2));
		return -1;
	}
	new_array = (struct memblock_region *)__va(addr);

	mm_debug("memblock:%s array is doubled to %p at[%p - %p]\n",
			memblock_type_name(type),
			(void *)(type->max * 2),
			(void *)(unsigned long)addr,
			(void *)(unsigned long)(addr + new_size - 1));

	/*
	 * Found space,we now need to move the array over before
	 * we add the reserved region since it may be our reserved
	 * array itself that is full.
	 */
	memcpy(new_array,type->regions,old_size);
	memset(new_array + type->max,0,old_size);
	old_array = type->regions;
	type->regions = new_array;
	type->max <<= 1;

	/* If we use SLUB that's it,we are done */
	if(use_slab)
		BUG_ON(memblock_add_region(&memblock.reserved,addr,new_size) < 0);

	/*
	 * If the array wasn't our static init one,then free it.We only do
	 * that before SLAB is available as later on,we don't know whether
	 * to use kfree or free_bootmem_pages().Shouldn't be a big deal
	 * anyways.
	 */
	if(old_array != memblock_memory_init_regions &&
		old_array != memblock_reserved_init_regions)
		memblock_free(__pa(old_array),old_size);

	return 0;
}

static long __init memblock_add_region(struct memblock_type *type,
		phys_addr_t base,phys_addr_t size)
{
	unsigned long coalesced = 0;
	long adjacent,i;
	
	if((type->cnt == 1) && (type->regions[0].size == 0)) {
		type->regions[0].base = base;
		type->regions[0].size = size;
		return 0;
	}

	/*
	 * First try and coalesce this MEMBLOCK with another.
	 */
	for(i = 0 ; i < type->cnt ; i++) {
		phys_addr_t rgnbase = type->regions[i].base;
		phys_addr_t rgnsize = type->regions[i].size;

		if((rgnbase == base) && (rgnsize == size))
			/* Already have this region,so we're done. */
			return 0;

		adjacent = memblock_addrs_adjacent(base,size,rgnbase,rgnsize);
		/* Check if arch allows coalescing */
		if(adjacent != 0 && type == &memblock.memory &&
				!memblock_memory_can_coalesce(base,size,rgnbase,rgnsize))
			break;
		if(adjacent > 0) {
			type->regions[i].base -= size;
			type->regions[i].size += size;
			coalesced++;
			break;
		} else if(adjacent < 0) {
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
	for(i = type->cnt - 1 ; i >= 0 ; i--) {
		if(base < type->regions[i].base) {
			type->regions[i + 1].base = type->regions[i].base;
			type->regions[i + 1].size = type->regions[i].size;
		} else {
			type->regions[i + 1].base = base;
			type->regions[i + 1].size = size;
			break;
		}
	}

	if(base < type->regions[0].base) {
		type->regions[0].base = base;
		type->regions[0].size = size;
	}
	type->cnt++;

	/*
	 * The array is full? Try to resize it.If that fails,we undo
	 * our allocation and return an erro.
	 */
	if(type->cnt == type->max && memblock_double_array(type)) {
		type->cnt--;
		return -1;
	}

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
long __init_memblock memblock_reserve(phys_addr_t base,phys_addr_t size)
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
	 * We align the size to limit fragmentation.Without this,a lot of
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
phys_addr_t __init memblock_alloc_base(phys_addr_t size,phys_addr_t align,
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





static int __init early_memblock(char *p)
{
	if(p && strstr(p,"debug"))
		memblock_debug = 1;
	return 0;
}
early_param("memblock",early_memblock);
