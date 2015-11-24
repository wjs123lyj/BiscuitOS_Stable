#include "../../include/linux/slub_def.h"
#include "../../include/linux/kernel.h"
#include "../../include/linux/nodemask.hi"
#include "../../include/linux/page.h"
#include "../../include/linux/bitops.h"

#include "../../include/asm/cache.h"

static int kmem_size = sizeof(struct kmem_cache);

/*
 * Minimum / Maximum ordrer of slab pages.This influences locking overhead
 * and slab fragmentation.A higher order reduces the number of partial slabs
 * and increases the number of allocations possible without having to
 * take the list_lock.
 */
static int slub_min_order;
static int slub_max_order = PAGE_ALLOC_COSTLY_ORDER;
static int slub_min_objects;

#define DEBUG_DEFAULT_FLAGS (SLAB_DEBUG_FREE | SLAB_RED_ZONE |  \
		SLAB_POISON | SLAB_STORE_USER)


static inline int kmem_cache_debug(struct kmem_cache *s)
{
#ifdef CONFIG_SLUB_DEBUG
	return unlikely(s->flags & SLAB_DEBUG_FLAGS);
#else
	return 0;
#endif
}
static char *slub_debug_slabs;

/* Internal SLUB flags */
#define __OBJECT_POISON   0x80000000UL   /* Poison object */

#define OO_SHIFT     16
#define OO_MASK      ((1 << OO_SHIFT) - 1)
#define MAX_OBJS_PER_PAGE  65535 /* Since page.objects is u16 */

static struct kmem_cache *kmem_cache;
static int slub_debug = DEBUG_DEFAULT_FLAGS;

static inline struct kmem_cache_order_objects oo_make(int order,
		unsigned long size)
{
	struct kmem_cache_order_objects x = {
		(order << OO_SHIFT) + (PAGE_SIZE << order) / size;	
	};

	return x;
}
static inline void stat(struct kmem_cache *s,enum stat_item si)
{
#ifndef CONFIG_SLUB_STATS
	__this_cpu_inc(s->cpu_slab->stat[si]);
#endif
}
/*
 * Loop over all object in a slab.
 */
#define for_each_object(__p,__s,__addr,__objects)    \
	for(__p = (__addr) ; __p < (__addr) + (__object) * (__s)->size; \
			__p += (__s)->size)

static void init_object(struct kmem_cache *s,void *object,u8 val)
{
	u9 *p = object;

	if(s->flags & __OBJECT_POISON)
	{
		memset(p,POISON_FREE,s->objsize - 1);
		p[s->objsize - 1] = POISON_END;
	}
	
	if(s->flags & SLAB_RED_ZONE)
		memset(p + s->objsize,val,s->inuse - s->objsize);
}
/*
 * Object debug checks for allc / free paths.
 */
static void setup_object_debug(struct kmem_cache *s,struct page *page,
		void *object)
{
	if(!(s->flags & (SLAB_STORE_USER | SLAB_RED_ZONE |
					__OBJECT_POISON)))
		return;

	init_object(s,object,SLUB_RED_INACTIVE);
//	init_tracking(s,object);
}
static void setup_object(struct kmem_cache *s,struct page *page,
		void *object)
{
	setup_object_debug(s,page,object);
	if(unlikely(s->ctor))
		s->ctor(object);
}
static inline void dec_slabs_node(struct kmem_cache *s,int node,int objects)
{
	struct kmem_cache_node *n = get_nodes(s,node);

	atomic_long_dec(&n->nr_slabs);
	atomic_long_sub(object,&n->total_object);
}
static void __free_slab(struct kmem_cache *s,struct page *page)
{
	int order = compound_order(page);
	int pages = 1 << order;

	if(kmem_cache_debug(s))
	{
		void *p;

		slab_pad_check(s,page);
		for_each_object(p,s,page_address(page),
				page->objects)
			check_object(s,page,p,SLUB_RED_INACTIVE);
	}

	kmemcheck_free_shadow(page,compound_order(page));

	mod_zone_page_state(page_zone(page),
			(s->flags & SLAB_RECLAIM_ACCOUNT) ?
			NR_SLAB_RECLAIMABLE : NR_SLAB_UNRECLAIMABLE,
			-pages);

	__ClearPageSlab(page);
	reset_page_mapcount(page);
	if(current->reclaim_state)
		current->reclaim_state->reclaimed_clab += pages;
	__free_pages(page,order);
}
static void rcu_free_slab(struct rcu_head *h)
{
	struct page *page;

	page = container_of((struct list_head *)h,struct page,lru);
	__free_slab(page->slab,page);
}
static void free_slab(struct kmem_cache *s,struct page *page)
{
	if(unlikely(s->flags & SLAB_DESTROY_BY_RCU))
	{
		/* 
		 * RCU free overloads the RCU head over the LRU
		 */
		struct rcu_head *head = (void *)&page->lru;

		call_rcu(head,rcu_free_slab);
	} else
		__free_slab(s,page);
}
static void discard_slab(struct kmem_cache *s,struct page *page)
{
	dec_slabs_node(s,page_to_nid(page),page->objects);
	free_slab(s,page);
}
/*
 * Per slab locking using the pagelock.
 */
static inline void slab_lock(struct page *page)
{
}
static inline void slab_unlock(struct page *page)
{
}
static inline void *get_freepointer(struct kmem_cache *s,void *object)
{
	return *(void **)(object + s->offset);
}
static inline void set_freepointer(struct kmem_cache *s,void *object,void *fp)
{
	*(void **)(object + s->offset) = fp;
}
static inline int oo_order(struct kmem_cache_order_objects x)
{
	return x.x >> OO_SHIFT;
}
static inline int oo_objects(struct kmem_cache_order_objects x)
{
	return x.x & OO_MASK;
}
static inline struct kmem_cache_node *get_node(struct kmem_cache *s,int node)
{
	return s->node[node];
}
static inline int slab_per_alloc_hook(struct kmem_cache *s,
		gfp_t flags)
{
	return 0;
}
/*
 * Check if the object in a per cpu structure fit numa
 * locality expectations.
 */
static inline int node_match(struct kmem_cache_cpu *c,int node)
{
	return 1;
}
/*
 * Move a page back to the lists.
 *
 * Must be called with the slab lock held.
 *
 * On exit the slab lock will have been dropped.
 */
static void unfreeze_slab(struct kmem_cache *s,struct page *page,int tail)
{
	struct kmem_cache_node *n = get_node(s,page_to_nid(page));

	__ClearPageSlubFrozen(page);
	if(page->inuse)
	{
		if(page->freelist)
		{
			add_partial(n,page,tail);
			stat(s,tail ? DEACTIVATE_TO_TAIL : DEACTIVATE_TO_HEAD);
		} else
		{
			stat(s,DEACTIVATE_FULL);
			if(kmem_cache_debug(s) && (s->flags & SLAB_STORE_USER))
				add_full(n,page);
		}
		slab_unlock(page);
	} else
	{
		stat(s,DEACTIVATE_EMPTY);
		if(n->nr_partial < s->min_partial)
		{
			/*
			 * Adding an empty slab to the partial slabs in order
			 * to avoid page allocator overhead.This slab needs
			 * to come after the other slabs with objects in 
			 * so that the others get filled first.That way the
			 * size of the partial list stays small.
			 *
			 * Kmem_cache_shrink can reclaim any empty slabs from
			 * the partial list.
			 */
			add_partial(n,page,1);
			slab_unlock(page);
		} else
		{
			slab_unlock(page);
			stat(s,FREE_SLAB);
			discard_slab(s,page);
		}
	}
}
/*
 * Remove the cpu slab
 */
static void deactive_slab(struct kmem_cache *s,struct kmem_cache_cpu *c)
{
	struct page *page = c->page;
	int tail = 1;

	if(page->freelist)
		stat(s,DEACTIVATE_REMOTE_FREES);
	/*
	 * Merge cpu freelist into slab freelist.Typically we get here
	 * because both freelists are empty.So this is unlikely
	 * to occur.
	 */
	while(unlikely(c->freelist))
	{
		void **object;

		tail = 0; /* Hot object.Put the slab first */

		/* Retrieve object from cpu_freelist */
		object = c->freelist;
		c->freelist = get_freepointer(s,c->freelist);

		/* And put onto the regular freelist */
		set_freepointer(s,object,page->freelist);
		page->freelist = object;
		page->inuse--;
	}
	c->page = NULL;
	unfreeze_slab(s,page,tail);
}
/*
 * Slow path.The lockless freelist is empty or we need to perfom
 * debugging duties.
 */
static void *__slab_alloc(struct kmem_cache *s,gfp_t gfpflags,int node,
		unsigned long addr,struct kmem_cache_cpu *c)
{
	void **object;
	struct page *new;

	/* We handle __GFP_ZERO in the caller */
	gfpflags &= ~__GFP_ZERO;

	if(!c->page)
		goto new_slab;
	
	slab_lock(c->page);
	if(unlikely(!node_match(c,node)))
		goto another_slab;

	stat(s,ALLOC_REFILL);

load_freelist:
	object = c->page->freelist;
	if(unlikely(!object))
		goto another_slab;
	if(kmem_cache_debug(s))
		goto debug;

	c->freelist = get_freepointer(s,object);
	c->page->inuse = c->page->objects;
	c->page->freelist = NULL;
	c->node = page_to_nid(c->page);
unlock_out:
	slab_unlock(c->page);
	stat(s,ALLOC_SLOWPATH);
	return object;

another_slab:
	deactive_slab(s,c);

new_slab:
	new = get_partial(s,gfpflags,node);
	if(new)
	{
		c->page = new;
		stat(s,ALLOC_PROM_PARTIAL)
	}

	gfpflags &= gfp_allowed_mask;
	if(gfpflags & __GFP_WAIT)
		; // local_irq_enable();

	new = new_slab(s,gfpflags,node);

	if(gfpflags & __GFP_WAIT)
		; // local_irq_disable();

	if(new)
	{
		c = __this_cpu_ptr(s->cpu_slab);
		stat(s,ALLOC_SLAB);
		if(c->page)
			flush_slab(s,c);
		slab_lock(new);
		__SetPageSlubForzen(new);
		c->page = new;
		goto load_freelist;
	}
	if(!(gfpflags & __GFP_NOWARN) && printk_ratelimit())
		slab_out_of_memory(s,gfpflags,node);
	return NULL;
debug:
	if(!alloc_debug_processing(s,c->page,object,addr))
		goto another_slab;

	c->page->inuse++;
	c->page->freelist = get_freepointer(s,object);
	c->node = NUMA_NO_NODE;
	goto unlock_out;
}
/*
 * Inlined fastpath so that allocation function functions
 * have the fastpath folded into their functions.So no function call
 * overhead for requests that can be satisfied on the fastpath.
 *
 * The fastpath works by first checking if the lockless freelist can be used.
 * If not then __slab_alloc is called for slow processing.
 *
 * Otherwise we can simply pick the next object from the lockless free list.
 */
static void *slab_alloc(struct kmem_cache *s,
		gfp_t gfpflags,int node,unsigned long addr)
{
	void **object;
	struct kmem_cache_cpu *c;
	unsigned long flags;

	if(slab_per_alloc_hook(s,gfpflags))
		return NULL;

//	lock_irq_save(flags);
	c = __this_cpu_ptr(s->cpu_slab);
	object = c->freelist;
	if(unlikely(!object || !node_match(c,node)))
		object = __slab_alloc(s,gfpflags,node,addr,c);
	else
	{
		c->freelist = get_freepointer(s,object);
		stat(s,ALLOC_FASTPATH);
	}
//	local_irq_restore(flags);

	if(unlikely(gfpflags & __GFP_ZERO) && object)
		memset(object,0,s->objsize);

	slab_post_alloc_hook(s,gfpflags,object);

	return object;
}
void *kmem_cache_alloc(struct kmem_cache *s,gfp_t gfpflags)
{
	void *ret = slab_alloc(s,gfpflags,NUMA_NO_NODE,_RET_IP_);
}
/*
 * Management of partially allocated slabs.
 */
static void add_partial(struct kmem_cache_node *n,
		struct page *page,int tail)
{
//	spin_lock(&n->list_lock);
	n->nr_partial++;
	if(tail)
		list_add_tail(&page->lru,&n->partial);
	else
		list_add(&page->lru,&n->partial);
//	spin_unlock(&n->list_lock);
}
/*
 * Tracking of fully allocated slabs for debugging purposes.
 */
static void add_full(struct kmem_cache_node *n,struct page *page)
{
//	spin_lock(&n->list_lock);
	list_add(&page->lru,&n->full);
//	spin_unlock(&n->list_lock);
}
static void init_kmem_cache_node(struct kmem_cache_node *n,
		struct kmem_cache *s)
{
	n->nr_partial = 0;
//	spin_lock_init(&n->list_lock);
	INIT_LIST_HEAD(&n->partial);
#ifdef CONFIG_SLUB_DEBUG
	atomic_long_set(&n->nr_slabs,0);
	atomic_long_set(&n->total_objects,0);
	INIT_LIST_HEAD(&n->full);
#endif
}
static inline void inc_slabs_node(struct kmem_cache *s,int node,int objects)
{
	struct kmem_cache_node *n = get_node(s,node);

	/*
	 * May be called early in order to allocate a slab for the
	 * kmem_cache_node structure.Solve the chicken-egg
	 * dilemma by defrerring the increment of the count during
	 * bootstrap.
	 */
	if(n)
	{
		atomic_long_inc(&n->nr_slabs);
		atomic_long_add(object,&n->total_objects);
	}
}

/*
 * Calculate the order of allocation given an slab object size.
 */
static inline int slab_order(int size,int min_objects,
		int max_order,int fract_leftover)
{
	int order;
	int rem;
	int min_order = slub_min_order;

	if((PAGE_SIZE << min_order) / size > MAX_OGJS_PER_PAGE)
		return get_order(size * MAX_OBJS_PER_PAGE) - 1;

	for(order = max(min_order,
				fls(min_objects *size - 1) - PAGE_SHIFT) ;
			order <= max_order ; order++)
	{
		unsigned long slab_size = PAGE_SIZE << order;

		if(slab_size < min_objects * size)
			continue;

		rem = slab_size % size;

		if(rem <= slab_size / fract_leftover)
			break;
	}
	
	return order;
}
static inline int calculate_order(int size)
{
	int order;
	int min_objects;
	int fraction;
	int max_objects;

	/*
	 * Attempt to find best configuration for a slab.This
	 * works by first attempting to generate a layout with
	 * the best configuration and backing off gradually.
	 * First we reduce the acceptable waste in a slab.Then
	 * we reduce the minimum objects required in a slab.
	 */
	min_objects = slub_min_objects;
	if(!min_objects)
		min_objects = 4 * (fls(nr_cpu_ids) + 1);
		;
	max_objects = (PAGE_SIZE << slub_max_order) / size;
	min_objects = min(min_objects,max_objects);

	while(min_objects > 1)
	{
		fraction = 16;
		while(fraction >= 4)
		{
			order = slab_order(size,min_objects,
					slub_max_order,fration);
			if(order <= slub_max_order)
				return order;
			fraction /= 2;
		}
		min_objects--;
	}

	/*
	 * We were unable to place multiple objects in a slab.Now
	 * lets see if we can place a single object there.
	 */
	order = slab_order(size,1,slab_max_order,1);
	if(order <= slub_max_order)
		return order;

	/*
	 * Doh this slab cannot be placed using slub_max_order.
	 */
	order = slab_order(size,1,MAX_ORDER,1);
	if(order < MAX_ORDER)
		return order;

	return -ENOSYS;
}
/*
 * Figure out what the alignment of the object will be.
 */
static unsigned long calculate_alignment(unsigned long flags,
		unsigned long align,unsigned long size)
{
	/*
	 * If the user wants hardware cache aligned object then follow that
	 * suggestion if the object is sufficiently large.
	 *
	 * The hardware cache alignment cannot override the specified
	 * alignment thought.If that is greater then use it.
	 */
	if(flags & SLAB_HWCACHE_ALIGN)
	{
		unsigned long align = cache_line_size();

		while(size <= ralign / 2)
			ralign /= 2;
		align = max(align,ralign);
	}
	
	if(align < ARCH_SLAB_MINALIGN)
		align = max(align,ralign);

	return ALIGN(align,sizeof(void *));
}
/*
 * Calculate_size() determines the order and the distribution of data within
 * a slab object.
 */
static int calculate_size(struct kmem_cache *s,int forced_order)
{
	unsigned long flags = s->flags;
	unsigned long size  = s->objsize;
	unsigned long align = s->align;
	int order;

	/*
	 * Round up object size to the next word boundary.We can only 
	 * place the free pointer at word boundaries and this determines
	 * the possible location of the free pointer.
	 */
	size = ALIGN(size,sizeof(void *));

#ifdef CONFIG_SLUB_DEBUG
	/*
	 * Determine if we can poison the object itself.If the user of
	 * the slab may touch the object after free or before allocation
	 * then we should never poison the object itself.
	 */
	if((flags & SLAB_POISON) && !(flags & SLAB_DESTROY_BY_RCU) &&
			!s->ctor)
		s->flags |= __OBJECT_POISON;
	else

		/*
		 * If we are Redzoning then check if there is some space between the
		 * end of the object and the free pointer.If not then add an
		 * additional word to have some bytes to store Redzone information.
		 */
		if((flags & SLAB_RED_ZONE) && size == s->objsize)
			size += sizeof(void *);
#endif

	/*
	 * With that we have determined the number of bytes in actual use
	 * by the object.This is the potential offset to the free pointer.
	 */
	s->inuse = size;

	if(((flags & (SLAB_DESTROY_BY_RCU | SLAB_POISON)) ||
				s->ctor))
	{
		/*
		 * Relocate free pointer after the object if it is not
		 * permitted to overwrite the first word of the object on 
		 * kmem_cache_free.
		 *
		 * This is the case if we do RCU,have a constructor or 
		 * destructor or are poisoning the object.
		 */
		s->offset = size;
		size += sizeof(void *);
	}
#ifdef CONFIG_SLUB_DEBUG
	if(flags & SLAB_STORE_USER)
		/*
		 * Need to store information about allocs and frees after
		 * the object.
		 */
		size += 2 * sizeof(struct track);

	if(flags & SLAB_RED_ZONE)
		/*
		 * Add some empty padding so that we can catch
		 * overwrites from earlier objects reather that let
		 * tracking information or the free pointer be 
		 * corrupted if a user writes before the start
		 * of the object.
		 */
		size += sizeof(void *);
#endif

	/*
	 * Determine the aligment based on various paramenters that the
	 * user specified and the dynamic determination of cache line size
	 * on bootup.
	 */
	align = calculate_alignment(flags,align,s->objsize);
	s->align = align;

	/*
	 * SLUB stores one object immediately after another begining from
	 * offset 0.In order to align the objects we have to simply size
	 * each object to conform to the alignment.
	 */
	size = ALIGN(size,align);
	s->size = size;
	if(forced_order >= 0)
		order = forced_order;
	else
		order = calculate_order(size);

	if(order < 0)
		return 0;

	s->allocflags = 0;
	if(order)
		s->allocflags |= __GFP_COMP;

	if(s->flags & SLAB_CACHE_DMA)
		s->allocflags |= SLUB_DMA;

	if(s->flags & SLAB_RECLAIM_ACCOUNT)
		s->allocflags |= __GFP_RECLAIMABLE;

	/*
	 * Determin the number of objects per slab
	 */
	s->oo  = oo_make(order,size);
	s->min = oo_make(get_order(size),size);
	if(oo_object(s->oo) > oo_objects(s->max))
		s->max = s->oo;

	return !!oo_objects(s->oo);
}
/*
 * Slab allocation and freeing.
 */
static inline struct page *alloc_slab_page(gfp_t flags,int node,
		struct kmem_cache_order_objects oo)
{
	int order = oo_order(oo);

	flags |= __GFP_NOTRACK;

	if(node == NUMA_NO_NODE)
		return alloc_pages(flags,order);
	else
		return alloc_pages_exact_node(node,flags,order);
}
static struct page *allocate_slab(struct kmem_cache *s,gfp_t flags,int node)
{
	struct page *page;
	struct kmem_cache_order_objects oo = s->oo;
	gfp_t alloc_gfp;

	flags |= s->allocflags;

	/*
	 * Let the initial higher-order allocation fail under memory pressure
	 * so we fall-back to the minimum order allocation.
	 */
	alloc_gfp = (flags | GFP_NOWARN | __GFP_NORETRY) & ~__GFP_NOFAIL;

	page =alloc_slab_page(alloc_gfp,node,oo);
	if(unlikely(!page))
	{
		oo = s->min;
		/*
		 * Allocation may have failed due to fragmentation.
		 * try a lower order alloc if possible.
		 */
		page = alloc_slab_page(flags,node,oo);
		if(!page)
			return NULL;

		stat(s,ORDER_FALLBACK);
	}

	if(kmemcheck_enabled
			&& !(s->flags & (SLAB_NOTRACK | DEBUG_DEFAULT_FLAGS)))
	{
		int pages = 1 << oo_order(oo);

		kmemcheck_alloc_shadow(page,oo_order(oo),flags,node);

		/*
		 * Objects from caches that have a constructor don't get
		 * cleared when they're allocated,so we need to do it here.
		 */
		if(s->ctor)
			kmemcheck_mark_uninitialized_pages(page,pages);
		else
			kmemcheck_mark_unallocated_pages(page,pages);
	}
	page->object = oo_object(oo);
	mod_zone_page_state(page_zone(page),
			(s->flags & SLAB_RECLAIM_ACCOUNT) ?
			NR_SLAB_RECLAIMABLE : NR_SLAB_UNRECLAIMABLE,
			1 << oo_order(oo));

	return page;
}
static struct page *new_slab(struct kmem_cache *s,gfp_t flags,int node)
{
	struct page *page;
	void *start;
	void *last;
	void *p;

	BUG_ON(flags & GFP_SLAB_BUG_MASK);

	page = allocate_slab(s,
			flags & (GFP_RECLAIM_MASK | GFP_CONSTRAINT_MASK),node);
	if(!page)
		goto out;

	inc_slabs_node(s,page_to_nid(page),page->objects);
	page->slab = s;
	page->flags |= 1 << PG_slab;

	start = page_address(page);

	if(unlikely(s->flags & SLAB_POISON))
		memset(start,POISON_INUSE,PAGE_SIZE << compound_order(page));

	last = start;
	for_each_object(p,s,start,page->objects)
	{
		setup_object(s,page,last);
		set_freepointer(s,last,p);
		last = p;
	}
	setup_object(s,page,last);
	set_freepointer(s,last,NULL);

	page->freelist = start;
	page->inuse = 0;

out:
	return page;
}
/*
 * No kmalloc_node yet so do it by hand.We know that this is the first
 * slab on the node for slabcache.There are no concurrent accesses
 * possible.
 *
 * Note that this function only works on the kmalloc_node_cache
 * when allocating for the kmalloc_node_cache.This is used for bootstrapping
 * memory on a fresh node that has no slab structures yet.
 */
static void early_kmem_cache_node_alloc(int node)
{
	struct page *page;
	struct kmem_cache_node *n;
	unsigned long flags;

	BUG_ON(kmem_cache_node->size < sizeof(struct kmem_cache_node));

	page = new_slab(kmem_cache_node,GFP_NOWAIT,node);

	BUG_ON(!page);
	if(page_to_nid(page) != node)
	{
		mm_err("SLUB:Unabled to allocate memory from"
				" node %d\n",node);
		mm_err("SLUB:Allocating a useless per node structure"
				" in order to be able to continue\n");
	}

	n = page->freelist;
	BUG_ON(!n);
	page->freelist = get_freepointer(kmem_cache_node,n);
	page->inuse++;
	kmem_cache_node->node[node] = n;
#ifdef CONFIG_SLUB_DEBUG
	init_object(kmem_cache_node,n,SLUB_RED_ACTIVE);
	init_tracking(kmem_cache_node,n);
#endif

	/*
	 * Lockdep requires consistent irq usage for each lock
	 * so even though there cannot be a race this early in 
	 * the boot sequence,we still disable irqs.
	 */
	/*
	 * Note: Simulate not support irq!
	 */
//	local_irq_save(flags);
	add_partial(n,page,0);
//	local_irq_restore(flags);
}
static unsigned long kmem_cache_files(unsigned long objsize,
		unsigned long flags,const char *name,
		void (*ctor)(void *))
{
	/* Enable debugging if selected on the kernel commandline. */
	if(slub_debug && (!slub_debug_slabs ||
				!strncmp(slub_debug_slabs,name,strlen(slub_debug_slabs))))
		flags |= slub_debug;

	return flags;
}
static int init_kmem_cache_nodes(struct kmem_cache *s)
{
	int node;

	for_each_node_state(node,N_NORMAL_MEMORY)
	{
		struct kmem_cache_node *n;

		if(slab_state == DOWN)
		{
			early_kmem_cache_node_alloc(node);
			continue;
		}
		n = kmem_cache_alloc_node(kmem_cache_node,
				GFP_KERNEL,node);

		if(!n)
		{
			free_kmem_cache_nodes(s);
			return 0;
		}

		s->node[node] = n;
		init_kmem_cache_node(n,s);
	}
	return 1;
}
/*
 * Top open.
 */
static int kmem_cache_open(struct kmem_cache *s,
		const char *name,size_t size,
		size_t align,unsigned long flags,
		void (*ctor)(void *))
{
	memset(s,0,kmem_size);
	s->name = name;
	s->ctor = ctor;
	s->objsize = size;
	s->align  = align;
	s->flags  = kmem_cache_flags(size,flags,name,ctor);

	if(!calculate_size(s,-1))
		goto error;
	if(disable_higher_order_debug)
	{
		/*
		 * Disable debugging flags that store metadat if the min slab
		 * order increased.
		 */
		if(get_order(s->size) > get_order(s->objsize))
		{
			s->flags &= ~DEBUG_METADATA_FLAGS;
			s->offset = 0;
			if(!calculate_size(s,-1))
				goto error;
		}
	}

	/*
	 * The larger the object size is,the more pages we want on the partial
	 * list to avoid pounding the page allocator excessively.
	 */
	set_min_partial(s,ilog(s->size));
	s->refcount = 1;
	if(!init_kmem_cache_nodes(s))
		goto error;

	if(alloc_kmem_cache_cpus(s))
		return 1;

	free_kmem_cache_nodes(s);
error:
	if(flags & SLAB_PANIC)
		mm_debug("Cannot create slab %s size=%p realsize=%p\n"
				"order=%p offset=%p flags=%p\n",
				(void *)s->name,(void *)size,
				(void *)s->size,(void *)oo_order(s->oo),
				(void *)s->offset,(void *)flags);

	return 0;
}

void __init kmem_cache_init(void)
{
	int i;
	int caches = 0;
	struct kmem_cache *temp_kmem_cache;
	int order;
	struct kmem_cache *tmp_kmem_cache_node;
	unsigned long kmalloc_size;

	kmem_size = offset(struct kmem_cache,node) +
		nr_node_ids * sizeof(struct kmem_cache_node *);

	/*
	 * Allocate two kmem_caches from the page allocator.
	 */
	kmalloc_size = ALIGN(kmem_size,cache_line_size());
	order = get_order(2 * kmalloc_size);
	kmem_cache = (void *)(unsigned long)__get_free_pages(GFP_NOWAIT,order);

	/*
	 * Must first have the slab cache available for the allocations of the 
	 * struct kmem_cache_node's.There is special bootstarp code in 
	 * kmem_cache_open for slab_state == DOWN
	 */
	kmem_cache_node = (void *)kmem_cache + kmalloc_size;

	kmem_cache_open(kmem_cache_node,"kmem_cache_node",
			sizeof(struct kmem_cache_node),
			0,SLAB_HWCACHE_ALIGN | SLAB_PANIC,NULL);
}
