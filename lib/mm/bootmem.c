#include "../../include/linux/config.h"
#include "../../include/linux/kernel.h"
#include "../../include/linux/bootmem.h"
#include "../../include/linux/mmzone.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/pfn.h"
#include "../../include/linux/error.h"
#include "../../include/linux/list.h"

#include "../../include/asm/dma.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned long max_low_pfn;
unsigned long min_low_pfn;
unsigned long max_pfn;

#define bootmem_arch_preferred_node(__bdata,size,align,goal,limit)     \
	(NODE_DATA(0)->bdata)
/*
 * Bootmem_data 
 */
#ifndef CONFIG_NO_BOOTMEM
struct bootmem_data bootmem_node_data[MAX_NUMNODES];
#endif
/*
 * The list of bootmem_data.
 */
struct list_head bdata_list = LIST_HEAD_INIT(bdata_list);

static unsigned long __init align_idx(struct bootmem_data *bdata,
		unsigned long idx,unsigned long step)
{
	unsigned long base = bdata->node_min_pfn;

	/*
	 * Align the index with respect to the node start so that the 
	 * combination of both satisfies the requested alignment.
	 */
	return ALIGN(base + idx,step) - base;
}
static unsigned long __init align_off(struct bootmem_data *bdata,
		unsigned long off,unsigned long align)
{
	unsigned long base = PFN_PHYS(bdata->node_min_pfn);

	/* Same as align_idx for byte offsets */
	return ALIGN(base + off, align) - base;
}
/*
 * free in bitmap.
 */
static void __init __free(struct bootmem_data *bdata,
		unsigned long sidx,unsigned long eidx)
{
	unsigned long idx;

	bdebug("nid=%td start=%lx end=%lx\n",
			bdata - bootmem_node_data,
			sidx + bdata->node_min_pfn,
			eidx + bdata->node_min_pfn);

	if(bdata->hint_idx > sidx)
		bdata->hint_idx = sidx;

	for(idx = sidx ; idx < eidx ; idx++)
		if(!test_and_clear_bit(idx,bdata->node_bootmem_map))
			BUG();
}
/*
 * reserved in bitmap.
 */
static int __init __reserve(struct bootmem_data *bdata,unsigned long sidx,
		unsigned long eidx,int flags)
{
	unsigned long idx;
	int exclusive = flags & BOOTMEM_EXCLUSIVE;

	bdebug("nid=%td start=%lx end=%lx flags=%x\n",
			bdata - bootmem_node_data,
			sidx + bdata->node_min_pfn,
			eidx + bdata->node_min_pfn,
			flags);

	for(idx = sidx ; idx < eidx ; idx++)
	{
		if(test_and_set_bit(idx,bdata->node_bootmem_map))
		{
			if(exclusive)
			{
				__free(bdata,sidx,idx);
				return -EBUSY;
			}
			bdebug("silent double reserve of PFN %lx\n",
					idx + bdata->node_min_pfn);
		}
	}
	return 0;
}
/*
 * Mark bootmem in some node.
 */
static int __init mark_bootmem_node(bootmem_data_t *bdata,
		unsigned long start,unsigned long end,
		int reserve,int flags)
{
	unsigned long sidx,eidx;

	bdebug("nid=%td start=%lx end=%lx reserve=%d flags=%x\n",
			bdata - bootmem_node_data,start,end,reserve,flags);

	BUG_ON(start < bdata->node_min_pfn);
	BUG_ON(end > bdata->node_low_pfn);

	sidx = start - bdata->node_min_pfn;
	eidx = end   - bdata->node_min_pfn;

	if(reserve)
		return __reserve(bdata,sidx,eidx,flags);
	else
		__free(bdata,sidx,eidx);
	return 0;
}
/*
 * Mark bootmem as free or reserve.
 */
static int __init mark_bootmem(unsigned long start,unsigned long end,
		int reserve,int flags)
{
	unsigned long pos;
	bootmem_data_t *bdata;

	pos = start;
	list_for_each_entry(bdata,&bdata_list,list)
	{
		int err;
		unsigned long max;

		if(pos < bdata->node_min_pfn ||
				pos >= bdata->node_low_pfn)
		{
			BUG_ON(pos != start);
			continue;
		}

		max = min(bdata->node_low_pfn,end);

		err = mark_bootmem_node(bdata,pos,max,reserve,flags);
		if(reserve && err)
		{
			mark_bootmem(start,pos,0,0);
			return err;
		}

		if(max == end)
			return 0;
		pos = bdata->node_low_pfn;
	}
	BUG();
}
/*
 * Free some area of bootmem.
 */
void __init free_bootmem(unsigned long addr,unsigned long size)
{
	unsigned long start,end;

	start = PFN_UP(addr);
	end   = PFN_DOWN(addr + size);

	mark_bootmem(start,end,0,0);
}
/*
 * Reserve some area of bootmem.
 */
int __init reserve_bootmem(unsigned long addr,unsigned long size,
		int flags)
{
	unsigned long start,end;

	start = PFN_DOWN(addr);
	end   = PFN_UP(addr + size);

	return mark_bootmem(start,end,1,flags);
}
/*
 * allocate bootmem core.
 */
static void * __init alloc_bootmem_core(struct bootmem_data *bdata,
		unsigned long size,unsigned long align,
		unsigned long goal,unsigned long limit)
{
	unsigned long fallback = 0;
	unsigned long min,max,start,sidx,midx,step;

	if(!bdata->node_bootmem_map)
		return NULL;

	min = bdata->node_min_pfn;
	max = bdata->node_low_pfn;

	goal >>= PAGE_SHIFT;
	limit >>= PAGE_SHIFT;

	if(limit && max > limit)
		max = limit;
	if(max <= min)
		return NULL;

	step = max(align >> PAGE_SHIFT,1UL);

	if(goal && min < goal && goal < max)
		start = ALIGN(goal,step);
	else
		start = ALIGN(min,step);

	sidx = start - bdata->node_min_pfn;
	midx = max - bdata->node_min_pfn;

	if(bdata->hint_idx > sidx)
	{
		/*
		 * Handle the valid case of sidx being zero and still
		 * catch the fallback below.
		 */
		fallback = sidx + 1;
		sidx = align_idx(bdata,bdata->hint_idx,step);
	}

	while(1)
	{
		int merge;
		void *region;
		unsigned long eidx,i,start_off,end_off;
find_block:
		sidx = find_next_zero_bit(bdata->node_bootmem_map,midx,sidx);
		sidx = align_idx(bdata,sidx,step);
		eidx = sidx + PFN_UP(size);

		if(sidx >= midx || eidx > midx)
			break;

		for(i = sidx ; i < eidx ; i++)
		{
			if(test_bit(i,bdata->node_bootmem_map))
			{
				sidx = align_idx(bdata,i,step);
				if(sidx == i)
					sidx += step;
				goto find_block;
			}
		}
		if(bdata->last_end_off & (PAGE_SIZE - 1) &&
				PFN_DOWN(bdata->last_end_off) + 1 == sidx)
			start_off = align_off(bdata,bdata->last_end_off,align);
		else
			start_off = PFN_PHYS(sidx);

		merge = PFN_DOWN(start_off) < sidx;
		end_off = start_off + size;

		bdata->last_end_off = end_off;
		bdata->hint_idx = PFN_UP(end_off);

		/*
		 * Reserve the area now.
		 */
		if(__reserve(bdata,PFN_DOWN(start_off) + merge,
					PFN_UP(end_off),BOOTMEM_EXCLUSIVE))
			mm_err("Err[%s - %d]\n",__FUNCTION__,__LINE__);
		region = (void *)(unsigned long)phys_to_virt(PFN_PHYS(bdata->node_min_pfn) + 
				start_off);
		/*
		 * In order to ignore the Virtual Memory,we indicate that
		 * will get virtual memory address when you request memory from system.
		 */
		memset(phys_to_mem(virt_to_phys(region)),0,size);

		return phys_to_mem(virt_to_phys(region));
	}
	if(fallback)
	{
		sidx = align_idx(bdata,fallback - 1,step);
		fallback = 0;
		goto find_block;
	}	
	return NULL;
}
static void * __init alloc_arch_preferred_bootmem(struct bootmem_data *bdata,
		unsigned long size,unsigned long align,
		unsigned long goal,unsigned long limit)
{
	struct bootmem_data *p_bdata;

	p_bdata = bootmem_arch_preferred_node(bdata,size,align,goal,limit);

	if(p_bdata)
		return alloc_bootmem_core(p_bdata,size,align,
				goal,limit);
}
static void * __init ___alloc_bootmem_nopanic(unsigned long size,
		unsigned long align,unsigned long goal,
		unsigned long limit)
{
	struct bootmem_data *bdata;
	void *region;

restart:
	region = alloc_arch_preferred_bootmem(NULL,size,align,goal,limit);
	if(region)
		return region;

	list_for_each_entry(bdata,&bdata_list,list)
	{
		if(goal && bdata->node_low_pfn <= PFN_DOWN(goal))
			continue;
		if(limit && bdata->node_min_pfn >= PFN_DOWN(limit))
			break;

		region = alloc_bootmem_core(bdata,size,align,goal,limit);
		if(region)
			return region;
	}
	if(goal)
	{
		goal = 0;
		goto restart;
	}
	return NULL;
}
static void * __init ___alloc_bootmem(unsigned long size,unsigned long align,
		unsigned long goal,unsigned long limit)
{
	void *mem = ___alloc_bootmem_nopanic(size,align,goal,limit);

	if(mem)
		return mem;
	/*
	 * Whoops,we cannot statisfy the allocation request.
	 */
	mm_err("Out of memory\n");
	return NULL;
}
static void * __init ___alloc_bootmem_node(struct bootmem_data *bdata,
		unsigned long size,unsigned long align,
		unsigned long goal,unsigned long limit)
{
	void *ptr;

	ptr = alloc_arch_preferred_bootmem(bdata,size,align,goal,limit);
	if(ptr)
		return ptr;

	ptr = alloc_bootmem_core(bdata,size,align,goal,limit);
	if(ptr)
		return ptr;

	return ___alloc_bootmem(size,align,goal,limit);
}
/*
 * __alloc_bootmem_node - allocate boot memory from a specific node
 */
void * __init __alloc_bootmem_node(struct pglist_data *pgdat,
		unsigned long size,unsigned long align,unsigned long goal)
{
	void *ptr;

	ptr = ___alloc_bootmem_node(pgdat->bdata,size,align,goal,0);

	return ptr;
}
