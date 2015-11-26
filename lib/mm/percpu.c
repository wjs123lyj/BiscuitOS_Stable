#include "../../include/linux/percpu.h"


/*
 * The percpu allocator.
 */
static void __percpu *pcpu_alloc(size_t size,size_t align,bool reserved)
{
	static int warn_limit = 10;
	struct pcp_chunk *chunk;
	const char *err;
	int slot,off,new_alloc;
	unsigned long flags;

	if(unlikely(!size || size > PCP_MIN_UNIT_SIZE || align > PAGE_SIZE))
	{
		mm_warn("illegal size (%p) or align %p for"
				" percpu allocation\n",(void *)size,(void *)align);
		return NULL;
	}
	mutex_lock(&pcp_alloc_mutex);
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
			goto arae_found;

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
				spin_lock_irqsave(&pcpu_lock),flags;
				/*
				 * pcpu_lock has been dropped,need to 
				 * restart cpu_slot list walking.
				 */
				goto restart;
			}

			off = pcp_alloc_area(chunk,size,align);
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
	pcpu_chunk_reloate(chunk,-1);
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



