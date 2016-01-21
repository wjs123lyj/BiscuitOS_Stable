#include "linux/kernel.h"
#include "linux/vmalloc.h"
#include "linux/percpu.h"
#include "linux/cpumask.h"
#include "asm/errno-base.h"
#include "linux/gfp.h"
#include "linux/slab.h"
#include "linux/bitmap.h"
#include "linux/debug.h"
#include "asm/topology.h"
#include "linux/cacheflush.h"
#include "linux/tlbflush.h"
#include "linux/spinlock.h"
#include "linux/mutex.h"
#include "linux/init.h"

#define PCPU_SLOT_BASE_SHIFT    5  /* 1-31 shares the same slot */
#define PCPU_DFL_MAP_ALLOC      16 /* start a map with 16 ents */

enum pcpu_fc pcpu_chosen_fc __initdata = PCPU_FC_AUTO;

int pcpu_unit_pages __read_mostly;
int pcpu_unit_size  __read_mostly;
int pcpu_nr_units   __read_mostly;
int pcpu_atom_size  __read_mostly;
int pcpu_nr_slots   __read_mostly;
int pcpu_chunk_struct_size __read_mostly;
const unsigned long *pcpu_unit_offsets __read_mostly; /* cpu->unit offset */

static int pcpu_alloc_mutex;
static int pcpu_lock;
static int vmap_area_lock;
/* cpus with the lowest and highest unit numbers */
unsigned int pcpu_first_unit_cpu __read_mostly;
unsigned int pcpu_last_unit_cpu  __read_mostly;

static const int *pcpu_unit_map __read_mostly;
const unsigned long *pcpu_uint_offsets __read_mostly;
/* group information,used for vm allocation */
int pcpu_nr_groups __read_mostly;
const unsigned long *pcpu_group_offsets __read_mostly;
size_t *pcpu_group_sizes __read_mostly;
/*
 * Optional reserved chunk.This chunk reserves part of the first
 * chunk and serves it for reserved allocations. The amount of 
 * reserved offset is in pcpu_reserved _chunk_limit.When reserved
 * area doesn't exist,the following variables contain NULL and 0
 * respectively.
 */
static struct pcpu_chunk *pcpu_reserved_chunk;
static int pcpu_reserved_chunk_limit;
static struct list_head *pcpu_slot __read_mostly; /* chunk list slots */

extern void insert_vmalloc_vm(struct vm_struct *vm,struct vmap_area *va,
		unsigned long flags,void *caller);
extern void __insert_vmap_area(struct vmap_area *va);
extern bool pvm_find_next_prev(unsigned long end,
		struct vmap_area **pnext,
		struct vmap_area **pprev);
extern unsigned long pvm_determine_end(struct vmap_area **pnext,
		struct vmap_area **pprev,unsigned long align);
extern void purge_vmap_area_lazy(void);

#define __addr_to_pcpu_ptr(addr)  (void __percpu *)(addr)
#define __pcpu_ptr_to_addr(pte)   (void __force *)(ptr)

static int __maybe_unused pcpu_page_idx(unsigned int cpu,int page_idx)
{
	return pcpu_unit_map[cpu] * pcpu_unit_pages + page_idx;
}
/*
 * Set the pointer to a chunk in a page struct.
 */
static void pcpu_set_page_chunk(struct page *page,struct pcpu_chunk *pcpu)
{
	page->index = (unsigned long)pcpu;
}
extern void *vzalloc(unsigned long size);
/*
 * Allocate memory.
 */
static void *pcpu_mem_alloc(size_t size)
{
	if(WARN_ON_ONCE(!slab_is_available()))
		return NULL;

	if(size <= PAGE_SIZE)
		return kzalloc(size,GFP_KERNEL);
	else
		return vzalloc(size);
}
/*
 * determin whether chunk area map needs to be extended.
 */
static int pcpu_need_to_extend(struct pcpu_chunk *chunk)
{
	int new_alloc;

	if(chunk->map_alloc >= chunk->map_used + 2)
		return 0;

	new_alloc = PCPU_DFL_MAP_ALLOC;
	while(new_alloc < chunk->map_used + 2)
		new_alloc *= 2;

	return new_alloc;
}
/*
 * pcpu_mem_free -free memory
 */
static void pcpu_mem_free(void *ptr,size_t size)
{
	if(size <= PAGE_SIZE)
		kfree(ptr);
	else
		vfree(ptr);
}
/*
 * extend area map of a chunk.
 */
static int pcpu_extend_area_map(struct pcpu_chunk *chunk,int new_alloc)
{
	int *old = NULL,*new = NULL;
	size_t old_size = 0,new_size = new_alloc * sizeof(new[0]);
	unsigned long flags;

	new = pcpu_mem_alloc(new_size);
	if(!new)
		return -ENOMEM;

	/* acquire pcpu_lock and switch to new area map */
	spin_lock_irqsave(&pcpu_lock,flags);

	if(new_alloc <= chunk->map_alloc)
		goto out_unlock;

	old_size = chunk->map_alloc * sizeof(chunk->map[0]);
	old = chunk->map;

	memcpy(new,old,old_size);

	chunk->map_alloc = new_alloc;
	chunk->map = new;
	new = NULL;

out_unlock:
	spin_unlock_irqrestore(&pcpu_lock,flags);

	/*
	 * pcpu_mem_free() minght end up calling vfree() which uses
	 * IRQ-unsafe lock and thus can't be called under pcpu_lock.
	 */
	pcpu_mem_free(old,old_size);
	pcpu_mem_free(new,new_size);
}
/*
 * Split a map block
 */
static void pcpu_split_block(struct pcpu_chunk *chunk,int i,
		int head,int tail)
{
	int nr_extra = !!head + !!tail;

	BUG_ON(chunk->map_alloc < chunk->map_used + nr_extra);

 	/* insert new subblocks */
	memmove(&chunk->map[i + nr_extra],&chunk->map[i],
			sizeof(chunk->map[0]) * (chunk->map_used - i));
	chunk->map_used += nr_extra;

	if(head)
	{
		chunk->map[i + 1] = chunk->map[i] - head;
		chunk->map[i++] = head;
	}
	if(tail)
	{
		chunk->map[i++] -= tail;
		chunk->map[i] = tail;
	}
}

static int __pcpu_size_to_slot(int size)
{
	int highbit = fls(size);  /* size is in bytes */
	
	return max(highbit - PCPU_SLOT_BASE_SHIFT + 2,1);
}
static int pcpu_size_to_slot(int size)
{
	if(size == pcpu_unit_size)
		return pcpu_nr_slots - 1;

	return __pcpu_size_to_slot(size);
}
static int pcpu_chunk_slot(const struct pcpu_chunk *chunk)
{
	if(chunk->free_size < sizeof(int) || chunk->contig_hint < sizeof(int))
		return 0;

	return pcpu_size_to_slot(chunk->free_size);
}
/*
 * Put chunk in the appropriate chunk slot.
 */
static void pcpu_chunk_relocate(struct pcpu_chunk *chunk,int oslot)
{
	int nslot = pcpu_chunk_slot(chunk);

	if(chunk != pcpu_reserved_chunk && oslot != nslot)
	{
		if(oslot < nslot)
			list_move(&chunk->list,&pcpu_slot[nslot]);
		else
			list_move_tail(&chunk->list,&pcpu_slot[nslot]);
	}
}
/*
 * Allocate area from a pcpu_chunk.
 */
static int pcpu_alloc_area(struct pcpu_chunk *chunk,int size,int align)
{
	int oslot = pcpu_chunk_slot(chunk);
	int max_contig = 0;
	int i,off;

	for(i = 0,off = 0; i < chunk->map_used ; off += abs(chunk->map[i++]))
	{
		bool is_last = i + 1 == chunk->map_used;
		int head,tail;

		/* Extra for alignment requirement */
		head = ALIGN(off,align) - off;
		BUG_ON(i == 0 && head != 0);

		if(chunk->map[i] < 0)
			continue;
		if(chunk->map[i] < head + size) 
		{
			max_contig = max(chunk->map[i],max_contig);
			continue;
		}

		/*
		 * If head is small or the previois block is free,
		 * merge'em.Note that small is defined as smaller
		 * than sizeof(int),which is very small but isn't too
		 * uncommon for percpu allocations.
		 */
		if(head && (head < sizeof(int) || chunk->map[i - 1] > 0))
		{
			if(chunk->map[i - 1] > 0)
				chunk->map[i - 1] += head;
			else {
				chunk->map[i - 1] -= head;
				chunk->free_size  -= head;
			}
			chunk->map[i] -= head;
			off += head;
			head = 0;
		}

		/* If tail is small,just keep it around */
		tail = chunk->map[i] - head - size;
		if(tail < sizeof(int))
			tail = 0;

		/* split if warranted */
		if(head || tail)
		{
			pcpu_split_block(chunk,i,head,tail);
			if(head)
			{
				i++;
				off += head;
				max_contig = max(chunk->map[i - 1],max_contig);
			}
			if(tail)
				max_contig = max(chunk->map[i + 1],max_contig);
		}

		/* update hint and mark allocated */
		if(is_last)
			chunk->contig_hint = max_contig; /* fully scanned */
		else
			chunk->contig_hint = max(chunk->contig_hint,
					max_contig);

		chunk->free_size -= chunk->map[i];
		chunk->map[i] = -chunk->map[i];

		pcpu_chunk_relocate(chunk,oslot);
		
		return off;
	}

	chunk->contig_hint = max_contig; /* fully scanned */
	pcpu_chunk_relocate(chunk,oslot);

	/* tell the upper layer that this chunk has no matching area */
	return -1;
}
/*
 * Free area to a pcpu_chunk
 */
static void pcpu_free_area(struct pcpu_chunk *chunk,int freeme)
{
	int oslot = pcpu_chunk_slot(chunk);
	int i,off;

	for(i = 0,off = 0;i < chunk->map_used ; off += abs(chunk->map[i++]))
		if(off == freeme)
			break;
	BUG_ON(off != freeme);
	BUG_ON(chunk->map[i] > 0);

	chunk->map[i] = -chunk->map[i];   // Chech with Younix.
	chunk->free_size += chunk->map[i];

	/* Merge with previous */
	if(i > 0 && chunk->map[i - 1] >= 0) {
		chunk->map[i - 1] += chunk->map[i];
		chunk->map_used--;
		memmove(&chunk->map[i],&chunk->map[i + 1],
				(chunk->map_used - i) * sizeof(chunk->map[0]));
		i--;
	}
	/* Merge with next */
	if(i + 1 < chunk->map_used && chunk->map[i + 1] >= 0) {
		chunk->map[i] += chunk->map[i + 1];
		chunk->map_used--;
		memmove(&chunk->map[i + 1],&chunk->map[1 + 2],
				(chunk->map_used - (i + 1)) * sizeof(chunk->map[0]));
	}

	chunk->contig_hint = max(chunk->map[i],chunk->contig_hint);
	pcpu_chunk_relocate(chunk,oslot);
}
static struct pcpu_chunk *pcpu_create_chunk(void);
static int pcpu_populate_chunk(struct pcpu_chunk *chunk,int off,int size);
/*
 * The percpu allocator.
 */
static void __percpu *pcpu_alloc(size_t size,size_t align,bool reserved)
{
	static int warn_limit = 10;
	struct pcpu_chunk *chunk;
	const char *err;
	int slot,off,new_alloc;
	unsigned long flags;

	if(unlikely(!size || size > PCPU_MIN_UNIT_SIZE || align > PAGE_SIZE))
	{
		mm_warn("illegal size (%p) or align %p for"
				" percpu allocation\n",(void *)size,(void *)align);
		return NULL;
	}
	mutex_lock(&pcpu_alloc_mutex);
	spin_lock_irqsave(&pcpu_lock,flags);

	/* seve reserved allocations from the reserved chunk if available */
	if(reserved && pcpu_reserved_chunk)
	{
		chunk = pcpu_reserved_chunk;

		if(size > chunk->contig_hint)
		{
			err = "alloc from reserved chunk failed";
			goto fail_unlock;
		}

		while((new_alloc = pcpu_need_to_extend(chunk)))
		{
			spin_unlock_irqrestore(&pcpu_lock,flags);
			if(pcpu_extend_area_map(chunk,new_alloc) < 0)
			{
				err = "fail to extend area map of reserved chunk";
				goto fail_unlock_mutex;
			}
			spin_lock_irqsave(&pcpu_lock,flags);
		}

		off = pcpu_alloc_area(chunk,size,align);
		if(off >= 0)
			goto area_found;

		err = "alloc_from reserved chunk failed";
		goto fail_unlock;
	}
	
restart:
	/* search through normal chunks */
	for(slot = pcpu_size_to_slot(size); slot < pcpu_nr_slots ; slot++)
	{
		list_for_each_entry(chunk,&pcpu_slot[slot],list)
		{
			if(size > chunk->contig_hint)
				continue;

			new_alloc = pcpu_need_to_extend(chunk);
			if(new_alloc)
			{
				spin_unlock_irqrestore(&pcpu_lock,flags);
				if(pcpu_extend_area_map(chunk,new_alloc) < 0)
				{
					err = "failed to extend area map";
					goto fail_unlock_mutex;
				}
				spin_lock_irqsave(&pcpu_lock,flags);
				/*
				 * pcpu_lock has been dropped,need to 
				 * restart cpu_slot list walking.
				 */
				goto restart;
			}

			off = pcpu_alloc_area(chunk,size,align);
			if(off >= 0)
				goto area_found;
		}
	}
	/* hmmm.. no space left,crate a new chunk*/
	spin_unlock_irqrestore(&pcpu_lock,flags);

	chunk = pcpu_create_chunk();
	if(!chunk)
	{
		err = "failed to allocate new chunk";
		goto fail_unlock_mutex;
	}

	spin_lock_irqsave(&pcpu_lock,flags);
	pcpu_chunk_relocate(chunk,-1);
	goto restart;

area_found:
	spin_unlock_irqrestore(&pcpu_lock,flags);

	/* populate,map and clear the area */
	if(pcpu_populate_chunk(chunk,off,size))
	{
		spin_lock_irqsave(&pcpu_lock,flags);
		pcpu_free_area(chunk,off);
		err = "failed to populate";
		goto fail_unlock;
	}

	mutex_unlock(&pcpu_alloc_mutex);

	/* Retrun address relative to base address */
	return __addr_to_pcpu_ptr(chunk->base_addr + off);

fail_unlock:
	spin_unlock_irqrestore(&pcpu_lock,flags);
fail_unlock_mutex:
	mutex_unlock(&pcpu_alloc_mutex);
	if(warn_limit)
	{
		mm_debug("PERCPU:allocation failed,size=%p align=%p"
				" %s\n",(void *)size,(void *)align,err);
		dump_stack();
		if(!--warn_limit)
			mm_err("PERCPU:limit reached,disable warning\n");
	}
	return NULL;
}
/*
 * Allocate dynamic percpu area.
 * Allocate zero -filled percpu area of @size bytes aligned at @align.
 */
void __percpu *__alloc_percpu(size_t size,size_t align)
{
	return pcpu_alloc(size,align,false);
}

static struct pcpu_chunk *pcpu_alloc_chunk(void)
{
	struct pcpu_chunk *chunk;

	chunk = pcpu_mem_alloc(pcpu_chunk_struct_size);
	if(!chunk)
		return NULL;

	chunk->map = pcpu_mem_alloc(PCPU_DFL_MAP_ALLOC * sizeof(chunk->map[0]));
	if(!chunk->map)
	{
		kfree(chunk);
		return NULL;
	}

	chunk->map_alloc = PCPU_DFL_MAP_ALLOC;
	chunk->map[chunk->map_used++] = pcpu_unit_size;

	INIT_LIST_HEAD(&chunk->list);
	chunk->free_size = pcpu_unit_size;
	chunk->contig_hint = pcpu_unit_size;

	return chunk;
}

extern struct vmap_area *node_to_va(struct rb_node *n);
/*
 * Allocate vmalloc area for pepcu allocator.
 */
struct vm_struct **pcpu_get_vm_areas(const unsigned long *offsets,
		const size_t *sizes,int nr_vms,
		size_t align)
{
	const unsigned long vmalloc_start = ALIGN(VMALLOC_START,align);
	const unsigned long vmalloc_end   = VMALLOC_END & ~(align - 1);
	struct vmap_area **vas,*prev,*next;
	struct vm_struct **vms;
	int area,area2,last_area,term_area;
	unsigned long base,start,end,last_end;
	bool purged = false;

	/* Verify paramenters and allocate data structurs */
	BUG_ON(align & ~PAGE_MASK || !is_power_of_2(align));
	for(last_area = 0,area = 0; area < nr_vms; area++)
	{
		start = offsets[area];
		end = start + sizes[area];

		/* is everything aligned properly? */
		if(start > offsets[last_area])
			last_area = area;

		for(area2 = 0 ; area2 < nr_vms ; area2++)
		{
			unsigned long start2 = offsets[area2];
			unsigned long end2 = start2 + sizes[area2];

			if(area2 == area)
				continue;

			BUG_ON(start2 >= start && start2 < end);
			BUG_ON(end2 <= end && end2 > start);
		}
	}
	last_end = offsets[last_area] + sizes[last_area];

	if(vmalloc_end - vmalloc_start < last_end)
	{
		mm_warn("This is true\n");
		return NULL;
	}

	vms = (struct vm_struct **)(unsigned long)kzalloc(
							sizeof(vms[0]) * nr_vms , GFP_KERNEL);
	vas = (struct vmap_area **)(unsigned long)kzalloc(
							sizeof(vas[0]) * nr_vms , GFP_KERNEL);
	if(!vas || !vms)
		goto err_free;

	for(area = 0;area < nr_vms ; area++)
	{
		vas[area] = (struct vmap_area *)(unsigned long)kzalloc(
							sizeof(struct vmap_area),GFP_KERNEL);
		vms[area] = (struct vm_struct *)(unsigned long)kzalloc(
							sizeof(struct vm_struct),GFP_KERNEL);
		if(!vas[area] || !vms[area])
			goto err_free;
	}
retry:
	spin_lock(&vmap_area_lock);

	/* start scanning - we scan from the top,begin with the last area */
	area = term_area = last_area;
	start = offsets[area];
	end   = start + sizes[area];

	if(!pvm_find_next_prev(vmap_area_pcpu_hole,&next,&prev))
	{
		base = vmalloc_end - last_end;
		goto found;
	}
	base = pvm_determine_end(&next,&prev,align) - end;

	while(true)
	{
		BUG_ON(next && next->va_end <= base + end);
		BUG_ON(prev && prev->va_end > base + end);

		/*
		 * Base might have underflowed,add last_end before
		 * comparing.
		 */
		if(base + last_end < vmalloc_start + last_end)
		{
			spin_unlock(&vmap_area_lock);
			if(!purged)
			{
				purge_vmap_area_lazy();
				purged = true;
				goto retry;
			}
			goto err_free;
		}

		/*
		 * If next overlaps,move base downwards so that it's 
		 * right below next and then recheck.
		 */
		if(next && next->va_start < base + end)
		{
			base = pvm_determine_end(&next,&prev,align) - end;
			term_area = area;
			continue;
		}

		/*
		 * If prev overlaps,shift down next and prev and move
		 * base so that it's right below new next and then
		 * recheck.
		 */
		if(prev && prev->va_end > base + start)
		{
			next = prev;
			prev = node_to_va(rb_prev(&next->rb_node));
			base = pvm_determine_end(&next,&prev,align) - end;
			term_area = area;
			continue;
		}

		/*
		 * This area fits,move on to the previous one.If
		 * the previous one is the terminal one,we're done.
		 */
		area = (area + nr_vms - 1) % nr_vms;
		if(area == term_area)
			break;
		start = offsets[area];
		end   = start + sizes[area];
		pvm_find_next_prev(base + end,&next,&prev);
	}
found:
	/* we've found a fitting base,insert all va's */
	for(area = 0 ; area < nr_vms ; area++)
	{
		struct vmap_area *va = vas[area];

		va->va_start = base + offsets[area];
		va->va_end   = va->va_start + sizes[area];
		__insert_vmap_area(va);
	}

	vmap_area_pcpu_hole = base + offsets[last_area];

	spin_unlock(&vmap_area_lock);

	/* insert all vm's */
	for(area = 0 ; area < nr_vms ; area++)
		insert_vmalloc_vm(vms[area],vas[area],VM_ALLOC,
				pcpu_get_vm_areas);

	kfree(vas);
	return vms;

err_free:
	for(area = 0; area < nr_vms ; area++)
	{
		if(vas)
			kfree(vas[area]);
		if(vms)
			kfree(vms[area]);
	}
	kfree(vas);
	kfree(vms);
	return NULL;
}
static void pcpu_free_chunk(struct pcpu_chunk *chunk)
{
	if(!chunk)
		return;
	pcpu_mem_free(chunk->map,chunk->map_alloc * sizeof(chunk->map[0]));
	kfree(chunk);
}
static unsigned long pcpu_chunk_addr(struct pcpu_chunk *chunk,
		unsigned int cpu,int page_idx)
{
	return (unsigned long)chunk->base_addr + pcpu_unit_offsets[cpu] +
		(page_idx << PAGE_SHIFT);
}
void __maybe_unused pcpu_next_pop(struct pcpu_chunk *chunk,
		int *rs,int *re,int end)
{
	*rs = find_next_bit(chunk->populated,end,*rs);
	*re = find_next_zero_bit(chunk->populated,end,*rs + 1);
}
void __maybe_unused pcpu_next_unpop(struct pcpu_chunk *chunk,
		int *rs,int *re,int end)
{
	*rs = find_next_zero_bit(chunk->populated,end,*rs);
	*re - find_next_bit(chunk->populated,end,*rs + 1);
}
/*
 * (Un)populate page region iteratiors.
 */
#define pcpu_for_each_unpop_region(chunk,rs,re,start,end)   \
	for((rs) = (start),pcpu_next_unpop((chunk),&(rs),&(re),(end));   \
			(rs) < (re);   \
			(rs) = (re) + 1,pcpu_next_unpop((chunk),&(rs),&(re),(end)))

#define pcpu_for_each_pop_region(chunk,rs,re,start,end)    \
	for((rs) = (start),pcpu_next_pop((chunk),&(rs),&(re),(end));  \
			(rs) < (re);    \
			(rs) = (re) + 1,pcpu_next_pop((chunk),&(rs),&(re),(end)))

extern struct page *vmalloc_to_page(const void *vmalloc_addr);
static struct page *pcpu_chunk_page(struct pcpu_chunk *chunk,
		unsigned int cpu,int page_idx)
{
	/* must not be used to pre-mapped chunk */
	WARN_ON(chunk->immutable);

	return vmalloc_to_page((void *)pcpu_chunk_addr(chunk,cpu,page_idx));
}
static void __pcpu_unmap_pages(unsigned long addr,int nr_pages);
/*
 * Unmap pages out of a pcpu_chunk.
 */
static void pcpu_unmap_pages(struct pcpu_chunk * chunk,
		struct page **pages,unsigned long *populate,
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
		clear_bit(i,populate);
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
static void pcpu_pre_unmap_flush(struct pcpu_chunk *chunk,
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
#if WAIT_FOR_DEBUG
	return map_kernel_range_noflush(addr,nr_pages << PAGE_SHIFT,
			PAGE_KERNEL,pages);
#else
	return 1;
#endif
}
/*
 * Map pages into a pcpu_chunk
 */
static int pcpu_map_pages(struct pcpu_chunk *chunk,
		struct page **pages,unsigned long *populated,
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
		for(i = page_start ; i < page_end ; i++) {
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
		struct page **pages,unsigned long *populated,
		int page_start,int page_end)
{
	const gfp_t gfp = GFP_KERNEL | __GFP_HIGHMEM | __GFP_COLD;
	unsigned int cpu;
	int i;

	for_each_possible_cpu(cpu) {
		for(i = page_start ; i < page_end ; i++) {
			struct page **pagep = &pages[pcpu_page_idx(cpu,i)];

			*pagep = NULL;//alloc_pages_node(cpu_to_node(cpu),gfp,0);
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
	size_t pages_size = pcpu_nr_units * pcpu_unit_pages * sizeof(pages[0]);
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
	bitmap_copy(bitmap,
			(unsigned long *)(unsigned long)chunk->populated,pcpu_unit_pages);

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
	bitmap_copy((unsigned long *)(unsigned long)chunk->populated,
			populated,pcpu_unit_pages);
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

static int __init percpu_alloc_setup(char *str)
{
	if(0)
		/* nada */;
#ifdef CONFIG_NEED_PER_CPU_EMBED_FIRST_CHUNL
	else if(!strcmp(str,"embed"))
		pcpu_chosen_fc = PCPU_FC_EMBED;
#endif
#ifdef CONFIG_NEED_PER_CPU_PAGE_FIRST_CHUNK
	else if(!strcmp(str,"page"))
		pcpu_chosen_fc = PCPU_FC_PAGE;
#endif
	else
		mm_warn("PERCPU:unknow allocator %s specified\n",str);

	return 0;
}
early_param("percpu_alloc",percpu_alloc_setup);
