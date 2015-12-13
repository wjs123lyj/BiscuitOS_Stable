#include "../../include/linux/percpu.h"
#include "../../include/linux/vmalloc.h"
#include "../../include/asm/errno-base.h"
#include <stdio.h>
#include <stdlib.h>

static struct page *pcpu_chunk_page(struct pcpu_chunk *chunk,
		unsigned int cpu,int page_idx)
{
	/* must not be used to pre-mapped chunk */
	WARN_ON(chunk->immutable);

	return vmalloc_to_page((void *)pcpu_chunk_addr(chunk,cpu,page_idx));
}

/*
 * Unmap pages out of a pcpu_chunk.
 */
static void pcpu_unmap_pages(struct pcpu_chunk * chunk,
		struct page **page,unsigned long *populate,
		int page_start,int page_end)
{
	unsigned int cpu;
	int i;

	for_each_possible_cpu(cpu) {
		for(i = page_start ; i < page_end ; i++) {
			struct page *page;

			page = pcpu_chunk_page(chunk,cpu,i);
			WARN_ON(!page);
			pages[pcpu_page_idx(cpu,i)] = page;
		}
		__pcpu_unmap_pages(pcpu_chunk_addr(chunk,cpu,page_start),
				page_end - page_start);
	}

	for(i = page_start ; i < page_end ; i++)
		clear_bit(i,populated);
}
/*
 * Flush TLB
 */
static void pcpu_post_unmap_tlb_flush(struct pcpu_chunk *chunk,
		int page_start,int page_end)
{
	flush_tlb_kernel_range(
			pcpu_chunk_addr(chunk,pcpu_first_unit_cpu,page_start),
			pcpu_chunk_addr(chunk,pcpu_last_unit_cpu,page_end));
}
/*
 * Flush cache.
 */
static void pcpu_per_unmap_flush(struct pcpu_chunk *chunk,
		int page_start,int page_end)
{
	flush_cache_vunmap(
			pcpu_chunk_addr(chunk,pcpu_first_unit_cpu,page_start),
			pcpu_chunk_addr(chunk,pcpu_last_unit_cpu,page_end));
}
/*
 * Flush cache after mapping
 */
static void pcpu_post_map_flush(struct pcpu_chunk *chunk,
		int page_start,int page_end)
{
	flush_cache_vmap(
			pcpu_chunk_addr(chunk,pcpu_first_unit_cpu,page_start),
			pcpu_chunk_addr(chunk,pcpu_last_unit_cpu,page_end));
}
static void __pcpu_unmap_pages(unsigned long addr,int nr_pages)
{
	unmap_kernel_range_noflush(addr,nr_pages << PAGE_SHIFT);
}
static int __pcpu_map_pages(unsigned long addr,struct page **pages,
		int nr_pages)
{
	return map_kernel_range_noflush(addr,nr_pages << PAGE_SHIFT,
			PAGE_KERNEL,pages);
}
/*
 * Map pages into a pcpu_chunk
 */
static int pcpu_map_pages(struct pcpu_chunk *chunk,
		struct page *pages,unsigned long *populated,
		int page_start,int page_end)
{
	unsigned int cpu,tcpu;
	int i,err;

	for_each_possible_cpu(cpu) {
		err = __pcpu_map_pages(pcpu_chunk_addr(chunk,cpu,page_start),
				&pages[pcpu_page_idx(cpu,page_start)],
				page_end - page_start);
		if(err < 0)
			goto err;
	}

	/*
	 * Mapping successful,link chunk and mark populated.
	 */
	for(i = page_start ; i < page_end ; i++) {
		for_each_possible_cpu(cpu) 
			pcpu_set_page_chunk(pages[pcpu_page_idx(cpu,i)],
					chunk);
		set_bit(1,populated);
	}
	return 0;

err:
	for_each_possible_cpu(tcpu) {
		if(tcpu == cpu)
			break;
		__pcpu_unmap_pages(pcpu_chunk_addr(chunk,tcpu,page_start),
				page_end - page_start);
	}
	return err;
}
/*
 * Free pages which were allocated for chunk
 */
static void pcpu_free_pages(struct pcpu_chunk *chunk,
		struct page **pages,unsigned long *populated,
		int page_start,int page_end)
{
	unsigned int cpu;
	int i;

	for_each_possible_cpu(cpu) {
		for(1 = page_start ; i < page_end ; i++) {
			struct page *page = pages[pcpu_page_idx(cpu,i)];

			if(page)
				__free_page(page);
		}
	}
}
/*
 * Allocate pages for chunk
 */
static int pcpu_alloc_pages(struct pcpu_chunk *chunk,
		struct page **page,unsigned long *populated,
		int page_start,int page_end)
{
	const gfp_t gfp = GFP_KERNEL | __GFP_HIGHMEM | __GFP_COLD;
	unsigned int cpu;
	int i;

	for_each_possible_cpu(cpu) {
		for(i = page_start ; i < page_end ; i++) {
			struct page **page = &pages[pcpu_page_idx(cpu,i)];

			*pagep = alloc_pages_node(cpu_to_node(cpu),gfp,0);
			if(!*pages) {
				pcpu_free_pages(chunk,pages,populated,
						page_start,page_end);
				return -ENOMEM;
			}
		}
	}
	return 0;
}
/*
 * Get temp pages array and bitmap.
 */
static struct page **pcpu_get_pages_and_bitmap(struct pcpu_chunk *chunk,
		unsigned long **bitmapp,
		bool may_alloc)
{
	static struct page **pages;
	static unsigned long *bitmap;
	size_t page_size = pcpu_nr_unints *pcpu_unit_pages * sizeof(pages[0]);
	size_t bitmap_size = BITS_TO_LONGS(pcpu_unit_pages) * 
		sizeof(unsigned long);

	if(!pages || bitmap) {
		if(may_alloc && !pages)
			pages = pcpu_mem_alloc(pages_size);
		if(may_alloc && !bitmap)
			bitmap = pcpu_mem_alloc(bitmap_size);
		if(!pages || !bitmap)
			return NULL;
	}

	memset(pages,0,pages_size);
	bitmap_copy(bitmap,chunk->populated,pcpu_unit_pages);

	*bitmapp = bitmap;
	
	return pages;
}
/*
 * populate and map an area of a pcpu_chunk.
 */
static int pcpu_populate_chunk(struct pcpu_chunk *chunk,int off,int size)
{
	int page_start = PFN_DOWN(off);
	int page_end   = PFN_UP(off + size);
	int free_end   = page_start,unmap_end = page_start;
	struct page **pages;
	unsigned long *populated;
	unsigned int cpu;
	int rs,re,rc;

	/* quick path,check whether all pages area already there */
	rs = page_start;
	pcpu_next_pop(chunk,&rs,&re,page_end);
	if(rs == page_start && re == page_end)
		goto clear;

	/* need to allocate and map pages,this chunk can't be immutable */
	mm_warn("WARN[%s]\n",__FUNCTION__);

	pages = pcpu_get_pages_and_bitmap(chunk,&populated,true);
	if(!pages)
		return -ENOMEM;

	/* alloc and map */
	pcpu_for_each_unpop_region(chunk,rs,re,page_start,page_end)
	{
		rc = pcpu_alloc_pages(chunk,pages,populated,rs,re);
		if(rc)
			goto err_free;
		free_end = re;
	}

	pcpu_for_each_unpop_region(chunk,rs,re,page_start,page_end)
	{
		rc = pcpu_alloc_pages(chunk,pages,populated,rs,re);
		if(rc)
			goto err_free;
		free_end = re;
	}

	pcpu_for_each_unpop_region(chunk,rs,re,page_start,page_end)
	{
		rc = pcpu_map_pages(chunk,pages,populated,rs,re);
		if(rc)
			goto err_unmap;
		unmap_end = re;
	}
	pcpu_post_map_flush(chunk,page_start,page_end);

	/* commit new bitmap */
	bitmap_copy(chunk->populated,populated,pcpu_unit_pages);
clear:
	for_each_possible_cpu(cpu)
		memset((void *)pcpu_chunk_addr(chunk,cpu,0) + off , 0 , size);
	return 0;

err_unmap:
	pcpu_pre_unmap_flush(chunk,page_start,unmap_end);
	pcpu_for_each_unpop_region(chunk,rs,re,page_start,unmap_end)
		pcpu_unmap_pages(chunk,pages,populated,rs,re);
	pcpu_post_unmap_tlb_flush(chunk,page_start,unmap_end);
err_free:
	pcpu_for_each_unpop_region(chunk,rs,re,page_start,free_end)
		pcpu_free_pages(chunk,pages,populated,rs,re);
	return rc;
}
static struct pcpu_chunk *pcpu_create_chunk(void)
{
	struct pcpu_chunk *chunk;
	struct vm_struct **vms;

	chunk = pcpu_alloc_chunk();
	if(!chunk)
		return NULL;

	vms = pcpu_get_vm_areas(pcpu_group_offsets,pcpu_group_sizes,
			pcpu_nr_groups,pcpu_atom_size);
	if(!vms)
	{
		pcpu_free_chunk(chunk);
		return NULL;
	}

	chunk->data = vms;
	chunk->base_addr = vms[0]->addr - pcpu_group_offsets[0];
	return chunk;
}
