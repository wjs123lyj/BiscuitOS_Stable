#include "linux/kernel.h"
#include "linux/cpumask.h"
#include "linux/slub_def.h"
#include "linux/percpu.h"
#include "linux/poison.h"
#include "linux/page-flags.h"
#include "linux/mm_types.h"
#include "linux/kmemcheck.h"
#include "linux/kmemleak.h"
#include "linux/gfp.h"
#include "asm/errno-base.h"
#include "asm/current.h"
#include "linux/swap.h"
#include "linux/mm.h"
#include "linux/rcupdate.h"
#include "linux/debug.h"
#include "asm/cache.h"
#include "asm/getorder.h"
#include "linux/log2.h"
#include "linux/atomic-long.h"
#include "linux/spinlock.h"
#include "linux/irqflags.h"
#include "linux/printk.h"
#include "linux/init.h"
#include "linux/kmemcheck.h"
#include "linux/bit_spinlock.h"
#include "linux/rcutiny.h"
#include "linux/lock.h"


/*
 * Minimum number of partial slabs.These will be left on the partial
 * lists even if they are empty.kmem_cache_shrink may reclaim them.
 */
#define MIN_PARTIAL 5
/*
 * Maximum number of desirable partial slabs.
 * The existence of more partial slabs makes kmem_cache_shrink
 * sort the partial list by the number of objects in the.
 */
#define MAX_PARTIAL 10

static enum {
	DOWN,    /* No slab functionality available */
	PARTIAL, /* Kmem_cache_node works */
	UP,      /* Everything works but does not show up in sysfs */
	SYSFS    /* Sysfs up */
} slab_state = DOWN;

static int kmem_size = sizeof(struct kmem_cache);

/*
 * Tracking user of a slab.
 */
struct track {
	unsigned long addr;  /* Called from address */
	int cpu;     /* Was running on cpu */
	int pid;
	unsigned long when; /* When did the operation */
};

enum track_item {
	TRACK_ALLOC,
	TRACK_FREE
};

char *kmalloc_name[] = {
	"kmalloc-8",
	"kmalloc-16",
	"kmalloc-32",
	"kmalloc-64",
	"kmalloc-128",
	"kmalloc-256",
	"kmalloc-512",
	"kmalloc-1024",
	"kmalloc-2048",
	"kmalloc-4096",
	"kmalloc-8192"
}; 
/*
 * Conversion table for small slab size / 8 to the index in the
 * kmalloc array.This is necessary for slab < 192 since we have non power
 * of two cache sizes there.The size of larger slabs can be determined
 * using fls.
 */
static s8 size_index[24] = {
	3,   /* 8 */
	4,   /* 16 */
	5,   /* 24 */
	5,   /* 32 */
	6,   /* 40 */
	6,   /* 48 */
	6,   /* 56 */
	6,   /* 64 */
	1,   /* 72 */
	1,   /* 80 */
	1,   /* 88 */
	1,   /* 96 */
	7,   /* 104 */
	7,   /* 112 */
	7,   /* 120 */
	7,   /* 128 */
	2,   /* 136 */
	2,   /* 144 */
	2,   /* 152 */
	2,   /* 160 */
	2,   /* 168 */
	2,   /* 176 */
    2,   /* 184 */
	2,   /* 192 */
};

static inline int size_index_elem(size_t bytes)
{
	return (bytes - 1) / 8;
}
/*
 * Minimum / Maximum ordrer of slab pages.This influences locking overhead
 * and slab fragmentation.A higher order reduces the number of partial slabs
 * and increases the number of allocations possible without having to
 * take the list_lock.
 */
static int slub_min_order;
static int slub_max_order = PAGE_ALLOC_COSTLY_ORDER;
static int slub_min_objects;
static struct kmem_cache *kmem_cache_node;
static int disable_higher_order_debug;
/* A list of all slab caches on the system */
static LIST_HEAD(slab_caches);

extern char *kstrdup(const char *s,gfp_t gfp);

#define down_write(x) do {} while(0)
#define up_write(x)   do {} while(0)

/*
 * Merge control.If this is set then no merging of slab caches will occur.
 * (Could be removed.This was introduced to pacify the merge skeptics.)
 */
static int slub_nomerge;

#define DEBUG_DEFAULT_FLAGS (SLAB_DEBUG_FREE | SLAB_RED_ZONE |  \
		SLAB_POISON | SLAB_STORE_USER)

#define DEBUG_METADATA_FLAGS (SLAB_RED_ZONE | SLAB_POISON |   \
		SLAB_STORE_USER)

#define SLAB_DEBUG_FLAGS (SLAB_RED_ZONE | SLAB_POISON | SLAB_STORE_USER | \
		SLAB_TRACE | SLAB_DEBUG_FREE)

/*
 * Set of flags that will prevent slab merging
 */
#define SLUB_NEVER_MERGE (SLAB_RED_ZONE | SLAB_POISON | SLAB_STORE_USER | \
		SLAB_TRACE | SLAB_DESTROY_BY_RCU | SLAB_NOLEAKTRACE | \
		SLAB_FAILSLAB)

#define SLUB_MERGE_SAME (SLAB_DEBUG_FREE | SLAB_RECLAIM_ACCOUNT | \
		SLAB_CACHE_DMA | SLAB_NOTRACK)

struct kmem_cache *kmalloc_caches[SLUB_PAGE_SHIFT];

#ifdef CONFIG_ZONE_DMA
static struct kmem_cache *kmalloc_dma_caches[SLUB_PAGE_SHIFT];
#endif

static inline int kmem_cache_debug(struct kmem_cache *s)
{
#ifdef CONFIG_SLUB_DEBUG
	return unlikely(s->flags & SLAB_DEBUG_FLAGS);
#else
	return 0;
#endif
}
static char *slub_debug_slabs;

void kfree(const void *x);
/* Internal SLUB flags */
#define __OBJECT_POISON   0x80000000UL   /* Poison object */

#define OO_SHIFT     16
#define OO_MASK      ((1 << OO_SHIFT) - 1)
#define MAX_OBJS_PER_PAGE  65535 /* Since page.objects is u16 */

static struct kmem_cache *kmem_cache;
static int slub_debug = DEBUG_DEFAULT_FLAGS;

#ifdef CONFIG_SYSFS

#else
static inline int sysfs_slab_add(struct kmem_cache *s) {return 0;}
static inline int sysfs_slab_alias(struct kmem_cache *s,const char *p)
{
	return 0; 
}
static inline void sysfs_slab_remove(struct kmem_cache *s)
{
	kfree(s->name);
	kfree(s);
}
#endif


static inline struct kmem_cache_order_objects oo_make(int order,
		unsigned long size)
{
	struct kmem_cache_order_objects x = {
		(order << OO_SHIFT) + (PAGE_SIZE << order) / size	
	};

	return x;
}
static inline void stat(struct kmem_cache *s,enum stat_item si)
{
#ifdef CONFIG_SLUB_STATS
	__this_cpu_inc(s->cpu_slab->stat[si]);
#endif
}
static inline void slab_post_alloc_hook(struct kmem_cache *s,gfp_t flags,
		void *object)
{
	flags &= gfp_allowed_mask;
	kmemcheck_slab_alloc(s,flags,object,s->objsize);
	kmemleak_alloc_recursive(object,s->objsize,1,s->flags,flags);
}
#ifdef CONFIG_SLUB_DEBUG
static inline void slab_free_hook(struct kmem_cache *s,void *x)
{
	kmemleak_free_recursive(x,s->flags);
}
#else
static inline void slab_free_hook(struct kmem_cache *s,void *x)
{
}
static inline void slab_free_hook_irq(struct kmem_cache *s,
		void *object) 
{
}
#endif
/*
 * Loop over all object in a slab.
 */
#define for_each_object(__p,__s,__addr,__objects)    \
	for(__p = (__addr) ; __p < (__addr) + (__objects) * (__s)->size; \
			__p += (__s)->size)

static void init_object(struct kmem_cache *s,void *object,u8 val)
{
	u8 *p = object;

	if(s->flags & __OBJECT_POISON)
	{
		memset(p,POISON_FREE,s->objsize - 1);
		p[s->objsize - 1] = POISON_END;
	}
	
	if(s->flags & SLAB_RED_ZONE)
		memset(p + s->objsize,val,s->inuse - s->objsize);
}

#ifdef CONFIG_SLUB_DEBUG
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
#else
static void setup_object_debug(struct kmem_cache *s,struct page *page,
		void *object) {}
#endif

static void setup_object(struct kmem_cache *s,struct page *page,
		void *object)
{
	setup_object_debug(s,page,object);
	if(unlikely(s->ctor))
		s->ctor(object);
}
static inline struct kmem_cache_node *get_node(struct kmem_cache *s,int node);
#ifdef CONFIG_SLUB_DEBUG
static inline void dec_slabs_node(struct kmem_cache *s,int node,int objects)
{
	struct kmem_cache_node *n = get_node(s,node);

	atomic_long_dec(&n->nr_slabs);
	atomic_long_sub(objects,&n->total_objects);
}
#else
static inline void dec_slabs_node(struct kmem_cache *s,int node,int objects)
{}
#endif


static void slab_err(struct kmem_cache *s,struct page *page,char *fmt,...)
{
}
#define isgraph(x)   1
static void print_section(char *text,u8 *addr,unsigned int length)
{
	int i,offset;
	int newline = 1;
	char ascii[17];

	ascii[16] = 0;

	for(i = 0 ; i < length ; i++)
	{
		if(newline)
		{
			mm_err("%s %p\n",text,addr + i);
			newline = 0;
		}
		mm_debug("%p",(void *)(unsigned long)addr[i]);
		offset = i % 16;
		ascii[offset] = isgraph(addr[i]) ? addr[i] : '.';
		if(offset == 15)
		{
			mm_debug(" %s\n",ascii);
			newline = 1;
		}
	}
	if(!newline)
	{
		i %= 16;
		while(i < 16)
		{
			mm_debug(" ");
			ascii[i] = ' ';
			i++;
		}
		mm_debug("%s\n",ascii);
	}
}
static u8 *check_bytes(u8 *start,unsigned int value,unsigned int bytes)
{
	while(bytes)
	{
		if(*start != (u8)value)
			return start;
		start++;
		bytes--;
	}
	return NULL;
}
static void restore_bytes(struct kmem_cache *s,char *message,u8 data,
		void *from,void *to)
{
}
int slab_is_available(void)
{
	return slab_state >= UP;
}
/*
 * Check the pad bytes at the end of a slab page.
 */
static int slab_pad_check(struct kmem_cache *s,struct page *page)
{
	u8 *start;
	u8 *fault;
	u8 *end;
	int length;
	int remainder;

	if(!(s->flags & SLAB_POISON))
		return 1;

	start = page_address(page);
	length = (PAGE_SIZE << compound_order(page));
	end = start + length;
	remainder = length % s->size;
	if(!remainder)
		return 1;

	fault = check_bytes(end - remainder,POISON_INUSE,remainder);
	if(!fault)
		return 1;
	while(end > fault && end[-1] == POISON_INUSE)
		end--;

	slab_err(s,page,"Padding overwriten.0x%p - 0x%p\n",fault,end - 1);
	print_section("Padding",end - remainder,remainder);

	restore_bytes(s,"slab padding",POISON_INUSE,end - remainder,end);

	return 0;
}
/*****************************************************
 *     Kmalloc subsystem
 *****************************************************/



static inline int check_object(struct kmem_cache *s,struct page *page,
		void *object,u8 val)
{
	return 1;
}

static struct kmem_cache *get_slab(size_t size,gfp_t flags);

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

	//__ClearPageSlab(page);
	reset_page_mapcount(page);
	if(current->reclaim_state)
		current->reclaim_state->reclaimed_slab += pages;
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
	bit_spin_lock(PG_locked,&page->flags);
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
static struct track *get_track(struct kmem_cache *s,void *object,
		enum track_item alloc)
{
	struct track *p;

	if(s->offset)
		p = object + s->offset + sizeof(void *);
	else
		p = object + s->inuse;

	return p + alloc;
}
static void print_track(const char *s,struct track *t)
{
	if(!t->addr)
		return;

	mm_debug("INFO:%s in %pS age=%p cpu=%p pid=%p\n",
			s,(void *)(unsigned long)t->addr,
			(void *)(unsigned long)t->when,
			(void *)(unsigned long)t->cpu,
			(void *)(unsigned long)t->pid);
}
static inline int slab_per_alloc_hook(struct kmem_cache *s,
		gfp_t flags)
{
	return 0;
}
static void print_tracking(struct kmem_cache *s,void *object)
{
	if(!(s->flags & SLAB_STORE_USER))
		return;
	print_track("Allocated",get_track(s,object,TRACK_ALLOC));
	print_track("Freed",get_track(s,object,TRACK_FREE));
}
static void print_page_info(struct page *page)
{
	mm_debug("INFO:Slab %p objects=%p used=%p\n"
			 "fp=%p flags=%p\n",
			 (void *)page,
			 (void *)(unsigned long)page->objects,
			 (void *)(unsigned long)page->inuse,
			 (void *)page->freelist,(void *)page->flags);
}
static void print_trailer(struct kmem_cache *s,struct page *page,u8 *p)
{
	unsigned int off;   /* Offset of last byte */
	u8 *addr = page_address(page);

	print_tracking(s,p);

	print_page_info(page);

	mm_debug("INFO:Object %p @offset= %p fp=%p\n",
			(void *)(unsigned long)p,
			(void *)(unsigned long)(p - addr),
			(void *)(unsigned long)get_freepointer(s,p));

	if(p > addr + 16)
		print_section("Bytes b4",p - 16, 16);
	
	print_section("Object",p,min_t(unsigned long,s->objsize,PAGE_SIZE));

	if(s->flags & SLAB_RED_ZONE)
		print_section("Redzone",p + s->objsize,
				s->inuse - s->objsize);

	if(s->offset)
		off = s->offset + sizeof(void *);
	else
		off = s->inuse;

	if(s->flags & SLAB_STORE_USER)
		off += 2 * sizeof(struct track);

	if(off != s->size)
		/* Beginning of the filler is the free pointer */
		print_section("Padding",p + off,s->size - off);

	dump_stack();
}
static void slab_bug(struct kmem_cache *s,char *fmt,...)
{
#if 0
	va_list args;
	char buf[100];

	va_start(args,fmt);
	vsnprintf(buf,sizeof(buf),fmt,args);
	va_end(args);
	mm_debug("=======================================\n");
	mm_debug("BUG %s:%s\n",s->name,buf);
	mm_debug("---------------------------------------\n");
#endif
}
static void object_err(struct kmem_cache *s,struct page *page,
		u8 *object,char *reason)
{
	slab_bug(s,"%s",reason);
	print_trailer(s,page,object);
}
/*
 * Check if the object in a per cpu structure fit numa
 * locality expectations.
 */
static inline int node_match(struct kmem_cache_cpu *c,int node)
{
	return 1;
}
static void slab_fix(struct kmem_cache *s,char *fmt,...)
{
#if 0
	va_list args;
	char buf[100];

	va_start(args,fmt);
	vsnprintf(buf,sizeof(buf),fmt,args);
	vm_end(args);
	mm_debug("FIX %s:%s\n",s->name,buf);
#endif
}
static void add_full(struct kmem_cache_node *n,struct page *page);
static void add_partial(struct kmem_cache_node *n,
		struct page *page,int tail);
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

	// Need debug
	//__ClearPageSlubFrozen(page);
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
static inline void flush_slab(struct kmem_cache *s,struct kmem_cache_cpu *c)
{
	stat(s,CPUSLAB_FLUSH);
	slab_lock(c->page);
	deactive_slab(s,c);
}

#ifdef CONFIG_SLUB_DEBUG
static inline unsigned long node_nr_slabs(struct kmem_cache_node *n)
{
	return atomic_long_read(&n->nr_slabs);
}
#else
static inline unsigned long node_nr_slabs(struct kmem_cache_node *n)
{
}
#endif
static int cont_free(struct page *page)
{
	return page->objects - page->inuse;
}
static inline unsigned long node_nr_objs(struct kmem_cache_node *n)
{
#ifdef CONFIG_SLUB_DEBUG
	return atomic_long_read(&n->total_objects);
#else
	return 0;
#endif
}
/*
 * Verify that a pointer has an address that is valid within a slab page.
 */
static inline int check_valid_pointer(struct kmem_cache *s,
		struct page *page,const void *object)
{
	void *base;

	if(!object)
		return 1;

	base = page_address(page);
	if(object < base || object >= base + page->objects * s->size ||
			(object - base) % s->size)
	{
		return 0;
	}
	
	return 1;
}
/*
 * Determine if a certain object on a page is on the freelist.Must hold the 
 * slab lock to guarantee that the chains are in a consistent state.
 */
static int on_freelist(struct kmem_cache *s,struct page *page,void *search)
{
	int nr = 0;
	void *fp = page->freelist;
	void *object = NULL;
	unsigned long max_objects;

	while(fp && nr <= page->objects)
	{
		if(fp == search)
			return 1;
		if(!check_valid_pointer(s,page,fp))
		{
			if(object)
			{
				object_err(s,page,object,
						"Freechain corrupt");
				set_freepointer(s,object,NULL);
				break;
			} else
			{
				slab_err(s,page,"Freepointer corrupt");
				page->freelist = NULL;
				page->inuse = page->objects;
				slab_fix(s,"Freelist cleared");
				return 0;
			}
			break;
		}
		object = fp;
		fp = get_freepointer(s,object);
		nr++;
	}
}
static int check_slab(struct kmem_cache *s,struct page *page)
{
	int maxobj;

	//if(!PageSlab(page)) Need debug
	if(0)
	{
		mm_err("Not a valid slab page\n");
		return 0;
	}

	maxobj = (PAGE_SIZE << compound_order(page)) / s->size;
	if(page->objects > maxobj)
	{
		mm_err("objects\n");
		return 0;
	}
	if(page->inuse > page->objects)
	{
		mm_err("inuse\n");
		return 0;
	}
	/* Slab_pad_check fixes things up after itself */
	slab_pad_check(s,page);
	
	return 1;
}
static int alloc_debug_processing(struct kmem_cache *s,struct page *page,
		void *object,unsigned long addr)
{
	if(!check_slab(s,page))
		goto bad;

	if(!on_freelist(s,page,object))
	{
		object_err(s,page,object,"Object already allocated");
		goto bad;
	}

	if(!check_valid_pointer(s,page,object))
	{
		object_err(s,page,object,"Freelist pointer check fails"); // 2015.11.25
		goto bad;
	}

	if(!check_object(s,page,object,SLUB_RED_INACTIVE))
		goto bad;

	/* Success perform special debug activities for allocs */
	if(s->flags & SLAB_STORE_USER)
		;
	//	set_track(s,object,TRACK_ALLOC,addr);
//	trace(s,page,object,1);
	init_object(s,object,SLUB_RED_ACTIVE);
	return 1;

bad:
	//if(PageSlab(page))   Need debug
	if(0)
	{
		/*
		 * If this is a slab page then let do the best we can
		 * to avoid issues in the future.Marking all objects
		 * as used avoids touching the remaining objects.
		 */
		slab_fix(s,"Marking all objects used");
		page->inuse = page->objects;
		page->freelist = NULL;
	}
	return 0;
}
static unsigned long count_partial(struct kmem_cache_node *n,
		int (*get_count)(struct page *))
{
	unsigned long flags;
	unsigned long x = 0;
	struct page *page;

	spin_lock_irqsave(&n->list_lock,flags);
	list_for_each_entry(page,&n->partial,lru)
		x += get_count(page);
	spin_unlock_irqrestore(&n->list_lock,flags);
	return x;
}
static int count_free(struct page *page)
{
	return page->objects - page->inuse;
}
static void slab_out_of_memory(struct kmem_cache *s,
		gfp_t gfpflags,int nid)
{
	int node;

	mm_debug("SLUB:Unable to allocate memory on node %d (gfp= 0x%x)\n",
			nid,gfpflags);
	mm_debug("cache:%s,object size:%d,buffer size:%d,"
			"default order:%d,min order:%d\n",s->name,s->objsize,
			s->size,oo_order(s->oo),oo_order(s->min));

	if(oo_order(s->min) > get_order(s->objsize))
		mm_debug(" %s debugging increased min order,use "
				"slub_debug = O to disable.\n",s->name);

	for_each_online_node(node)
	{
		struct kmem_cache_node *n = get_node(s,node);
		unsigned long nr_slabs;
		unsigned long nr_objs;
		unsigned long nr_free;

		if(!n)
			continue;

		nr_free   = count_partial(n,count_free);
		nr_slabs  = node_nr_slabs(n);
		nr_objs   = node_nr_objs(n);

		mm_debug(" node %p:slabs:%p,objs:%p,free:%p\n",
				(void *)(unsigned long)node,
				(void *)(unsigned long)nr_slabs,
				(void *)(unsigned long)nr_objs,
				(void *)(unsigned long)nr_free);
	}
}

static inline int slab_trylock(struct page *page)
{
	int rc = 1;

	rc = bit_spin_trylock(PG_locked,&page->flags);
	return rc;
}

static inline void __remove_partial(struct kmem_cache_node *n,
		struct page *page);

/*
 * Lock slab and remove from the partial list.
 *
 * Must hold list_lock.
 */
static inline int lock_and_freeze_slab(struct kmem_cache_node *n,
		struct page *page)
{
	if(slab_trylock(page)) {
		__remove_partial(n,page);
		__SetPageSlubFrozen(page);
		return 1;
	}
	return 0;
}

/*
 * Try to allocate a partial slab from a specific node.
 */
static struct page *get_partial_node(struct kmem_cache_node *n)
{
	struct page *page;

	/*
	 * Racy check.If we mistakenly see no partial slabs then we
	 * just allocate an empty slab.If we mistakenly try to get a 
	 * partial slab and there is none available then get_partials()
	 * will return NULL.
	 */
	if(!n || !n->nr_partial)
		return NULL;

	spin_lock(&n->list_lock);
	list_for_each_entry(page,&n->partial,lru)
		if(lock_and_freeze_slab(n,page))
			goto out;
	page = NULL;
out:
	spin_unlock(&n->list_lock);
	return page;
}

/*
 * Get a page from somewhere.Search in increasing NUMA distances.
 */
static struct page *get_any_partial(struct kmem_cache *s,gfp_t flags)
{
	return NULL;
}

/*
 * Get a partial page,lock it and return it.
 */
static struct page *get_partial(struct kmem_cache *s,gfp_t flags,int node)
{
	struct page *page;
	int searchnode = (node == NUMA_NO_NODE) ? numa_node_id() : node;

	page = get_partial_node(get_node(s,searchnode));
	if(page || node != -1)
		return page;

	return get_any_partial(s,flags);
}

static struct page *new_slab(struct kmem_cache *s,gfp_t flags,int node);

/*
 * Slow path.The lockless freelist is empty or we need to perform
 * debugging duties.
 *
 * Interrupts are disalbed
 * 
 * Processing is still very fast if new objects have been freed to the
 * regular freelist.In that case we simply take over the regular freelist
 * as the lockless freelist and zap the regular freelist.
 * 
 * If that is not working then we fall back to the partial lists.We take the
 * first element of the freelist as the object to allocate now and move the
 * rest of the freelist to the lockless freelist.
 *
 * And if we were unable to get a new slab from the partial slab lists then
 * we needs to allocate a new slab.This is the slowest path since it involves
 * a call to the page allocator and the setup of a new slab.
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
	if(new) {
		c->page = new;
		stat(s,ALLOC_FROM_PARTIAL);
		goto load_freelist;
	}

	gfpflags &= gfp_allowed_mask;
	if(gfpflags & __GFP_WAIT)
		local_irq_enable();

	new = new_slab(s,gfpflags,node);

	if(gfpflags & __GFP_WAIT)
		local_irq_disable();

	if(new) {
		c = __this_cpu_ptr(s->cpu_slab);
		stat(s,ALLOC_SLAB);
		if(c->page)
			flush_slab(s,c);
		slab_lock(new);
		__SetPageSlubFrozen(new);
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

	lock_irq_save(flags);
	c = __this_cpu_ptr(s->cpu_slab);
	object = c->freelist;
	if(unlikely(!object || !node_match(c,node)))
		
		object = __slab_alloc(s,gfpflags,node,addr,c);

	else {
		c->freelist = get_freepointer(s,object);
		stat(s,ALLOC_FASTPATH);
	}
	local_irq_restore(flags);

	if(unlikely(gfpflags & __GFP_ZERO) && object)
		memset(object,0,s->objsize);

	slab_post_alloc_hook(s,gfpflags,object);

	return object;
}

void *__kmalloc(size_t size,gfp_t flags)
{
	struct kmem_cache *s;
	void *ret;

	if(unlikely(size > SLUB_MAX_SIZE))
		return kmalloc_large(size,flags);

	s = get_slab(size,flags);

	if(unlikely(ZERO_OR_NULL_PTR(s)))
		return s;

	ret = slab_alloc(s,flags,NUMA_NO_NODE,_RET_IP_);

	return ret;
}

void *kmem_cache_alloc(struct kmem_cache *s,gfp_t gfpflags)
{
	void *ret = slab_alloc(s,gfpflags,NUMA_NO_NODE,_RET_IP_);

	return ret;
}
/*
 * Management of partially allocated slabs.
 */
static void add_partial(struct kmem_cache_node *n,
		struct page *page,int tail)
{
	spin_lock(&n->list_lock);
	n->nr_partial++;
	if(tail)
		list_add_tail(&page->lru,&n->partial);
	else 
		list_add(&page->lru,&n->partial);
	spin_unlock(&n->list_lock);
}
static inline void __remove_partial(struct kmem_cache_node *n,
		struct page *page)
{
	list_del(&page->lru);
	n->nr_partial--;
}
static void remove_partial(struct kmem_cache *s,struct page *page)
{
	struct kmem_cache_node *n = get_node(s,page_to_nid(page));

	spin_lock(&n->list_lock);
	__remove_partial(n,page);
	spin_unlock(&n->list_lock);
}

#ifdef CONFIG_SLUB_DEBUG
/*
 * Tracking of fully allocated slabs for debugging purposes.
 */
static void add_full(struct kmem_cache_node *n,struct page *page)
{
	spin_lock(&n->list_lock);
	list_add(&page->lru,&n->full);
	spin_unlock(&n->list_lock);
}
#else
static void add_full(struct kmem_cache_node *n,struct page *page)
{
}

static inline unsigned long slabs_node(struct kmem_cache *s,int node) 
{
	return 0;
}
#endif


static void init_kmem_cache_node(struct kmem_cache_node *n,
		struct kmem_cache *s)
{
	n->nr_partial = 0;
	spin_lock_init(&n->list_lock);
	INIT_LIST_HEAD(&n->partial);
#ifdef CONFIG_SLUB_DEBUG
	atomic_long_set(&n->nr_slabs,0);
	atomic_long_set(&n->total_objects,0);
	INIT_LIST_HEAD(&n->full);
#endif
}

#ifdef CONFIG_SLUB_DEBUG
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
		atomic_long_add(objects,&n->total_objects);
	}
}
#else
static inline void inc_slabs_node(struct kmem_cache *s,int node,
		int objects) {}
#endif


/*
 * Calculate the order of allocation given an slab object size.
 *
 * The order of allocation has significant impact on performance and other
 * system components.Generally order 0 allocations should be preferred since
 * order 0 does not cause fragmentation in the page allocator.Large objects
 * be problematic to put into order 0 slabs because there may be too much
 * unused space left.We go to a higher order if more than 1/16th of the slab
 * would be wasted.
 *
 * In order to reach satisfactory performance we must ensure that a minium
 * number of objects is in one slab.Otherwise we may generate too much
 * activity on the partial lists which requires taking the list_lock.This is
 * less a concern for large slabs though which are rarely used.
 *
 * slub_max_order specifies the order where we begin to stop considering the
 * number of objects in a slab as critical.If we reach slub_max_order then
 * we try to keep the page order as low as possible.So we accept more waste
 * of space in favor of a small page order.
 *
 * Higher order allocations also allow the placement of more objects in a
 * slab and thereby reduce onject handing overhead.If the user has
 * requested a higher mininum order then we start with one instead of
 * the smallest order which wil fit the object.
 */
static inline int slab_order(int size,int min_objects,
		int max_order,int fract_leftover)
{
	int order;
	int rem;
	int min_order = slub_min_order;

	if((PAGE_SIZE << min_order) / size > MAX_OBJS_PER_PAGE)
		return get_order(size * MAX_OBJS_PER_PAGE) - 1;

	/**
	 * Need more debug...fls(?) -> slub_min_order ?= !0.
	 */
	for(order = max(min_order,
				fls(min_objects * size - 1) - PAGE_SHIFT) ;
			order <= max_order ; order++) {

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
	 *
	 * First we reduce the acceptable waste in a slab.Then
	 * we reduce the minimum objects required in a slab.
	 */
	min_objects = slub_min_objects;
	if(!min_objects)
		min_objects = 4 * (fls(nr_cpu_ids) + 1);
	max_objects = (PAGE_SIZE << slub_max_order) / size;
	min_objects = min(min_objects,max_objects);

	while(min_objects > 1) {
		fraction = 16;
		while(fraction >= 4) {
			order = slab_order(size,min_objects,
					slub_max_order,fraction);
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
	order = slab_order(size,1,slub_max_order,1);
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
static void *kmalloc_large_node(size_t size,gfp_t flags,int node)
{
	struct page *page;
	void *ptr = NULL;

	flags |= __GFP_COMP | __GFP_NOTRACK; 
	page = alloc_pages_node(node,flags,get_order(size));
	if(page)
		ptr = page_address(page);

	kmemleak_alloc(ptr,size,1,flags);
	return ptr;
}

static struct kmem_cache *get_slab(size_t size,gfp_t flags)
{
	int index;

	if(size <= 192) {
		if(!size)
			return ZERO_SIZE_PTR;
	
		index = size_index[size_index_elem(size)];
	} else
		index = fls(size - 1);

	return kmalloc_caches[index];
}
void *__kmalloc_node(size_t size,gfp_t flags,int node)
{
	struct kmem_cache *s;
	void *ret;

	if(unlikely(size > SLUB_MAX_SIZE))
	{
		ret = kmalloc_large_node(size,flags,node);

#if 0
		trace_kmalloc_node(_RET_IP_,ret,
				size,PAGE_SIZE << get_order(size),
				flags,node);
#endif	
		return ret;
	}

	s = get_slab(size,flags);

	if(unlikely(ZERO_OR_NULL_PTR(s)))
		return s;

	ret = slab_alloc(s,flags,node,_RET_IP_);

	/* Need debug */
	//trace_kmalloc_node(_RET_IP_,ret,size,s->size,flags,node);

	return ret;
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
	if(flags & SLAB_HWCACHE_ALIGN) {
		unsigned long ralign = cache_line_size();

		while(size <= ralign / 2)
			ralign /= 2;
		align = max(align,ralign);
	}

	if(align < ARCH_SLAB_MINALIGN)
		align = ARCH_SLAB_MINALIGN;

	return ALIGN(align,sizeof(void *));
}

/*
 * Calculate_size() determines the order and the distribution of data within
 * a slab object.
 */
static int calculate_sizes(struct kmem_cache *s,int forced_order)
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
	/**
	 * Need more debug...For this system,we use 32bit address!!!
	 * If size is small than 8? (ALIGN() up alignment!)
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
		s->flags &= ~__OBJECT_POISON;


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
				s->ctor)) {
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
	if(oo_objects(s->oo) > oo_objects(s->max))
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
	alloc_gfp = (flags | __GFP_NOWARN | __GFP_NORETRY) & ~__GFP_NOFAIL;

	page =alloc_slab_page(alloc_gfp,node,oo);
	if(unlikely(!page)) {
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
			&& !(s->flags & (SLAB_NOTRACK | DEBUG_DEFAULT_FLAGS))) {
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

	page->objects = oo_objects(oo);
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
	void *test;

	BUG_ON(flags & GFP_SLAB_BUG_MASK);

	page = allocate_slab(s,
			flags & (GFP_RECLAIM_MASK | GFP_CONSTRAINT_MASK),node);
	if(!page)
		goto out;

	inc_slabs_node(s,page_to_nid(page),page->objects);
	page->slab = s;
	page->flags |= 1 << PG_slab;

	/* In order to simulate... */
	start = phys_to_mem(__pa(page_address(page)));

	if(unlikely(s->flags & SLAB_POISON))
		memset(start,POISON_INUSE,PAGE_SIZE << compound_order(page));

	last = start;
	for_each_object(p,s,start,page->objects) {
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
	if(page_to_nid(page) != node) {
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
#endif
	init_kmem_cache_node(n,kmem_cache_node);
	inc_slabs_node(kmem_cache_node,node,page->objects);
	/*
	 * Lockdep requires consistent irq usage for each lock
	 * so even though there cannot be a race this early in 
	 * the boot sequence,we still disable irqs.
	 */
	local_irq_save(flags);
	add_partial(n,page,0);
	local_irq_restore(flags);
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
static inline int free_debug_processing(struct kmem_cache *s,
		struct page *page,void *object,unsigned long addr)
{
	return 0;
}
/*
 * Slow pathch handing.This may still be called frequently since objects
 * have a longer lifetime than the cpu slabs in most processing loads.
 *
 * So we still attempt to reduce cache line usage.Just take the slab
 * lock and free the item.If there is no additional partial page
 * handing required then we can return immediately.
 */
static void __slab_free(struct kmem_cache *s,struct page *page,
		void *x,unsigned long addr)
{
	void *prior;
	void **object = (void *)x;

	
	stat(s,FREE_SLOWPATH);
	slab_lock(page);

	if(kmem_cache_debug(s))
		goto debug;

check_ok:
	prior = page->freelist;
	set_freepointer(s,object,prior);
	page->freelist = object;
	page->inuse--;

	/* Need debug */
	//if(unlikely(PageSlubFrozen(page)))
	if(0)
	{
		stat(s,FREE_FROZEN);
		goto out_unlock;
	}

	if(unlikely(!page->inuse))
		goto slab_empty;

	/*
	 * Objects left in the slab.If it was not on the partial list before
	 * then add it.
	 */
	if(unlikely(!prior))
	{
		add_partial(get_node(s,page_to_nid(page)),page,1);
		stat(s,FREE_ADD_PARTIAL);
	}

out_unlock:
	slab_unlock(page);
	return;

slab_empty:
	if(prior)
	{
		/*
		 * Slab still on the partial list.
		 */
		remove_partial(s,page);
		stat(s,FREE_REMOVE_PARTIAL);
	}
	slab_unlock(page);
	stat(s,FREE_SLAB);
	discard_slab(s,page);
	return;

debug:
	if(!free_debug_processing(s,page,x,addr))
		goto out_unlock;
	goto check_ok;
}
/*
 * Fastpath with forced inlining to produce a kfree and kmem_cache_free that
 * can perform fastpath freeing without additional function calls.
 *
 * The fastpath is only possible if we are freeing to the current cpu slab
 * of this processor.This typically the case if we have just allocated
 * the item before.
 *
 * If fastpath is not possible then fall back to  __slab_free where we deal
 * with all sorts of special processing.
 */
static inline void slab_free(struct kmem_cache *s,
		struct page *page,void *x,unsigned long addr)
{
	void **object = (void *)x;
	struct kmem_cache_cpu *c;
	unsigned long flags;

	slab_free_hook(s,x);

	local_irq_save(flags);
	c = (struct kmem_cache_cpu *)__this_cpu_ptr(s->cpu_slab);

	slab_free_hook_irq(s,x);

	if(likely(page == c->page && c->node != NUMA_NO_NODE)) {
		set_freepointer(s,object,c->freelist);
		c->freelist = object;
		stat(s,FREE_FASTPATH);
	} else
		__slab_free(s,page,x,addr);

//	local_irq_restore(flags);
}

void kfree(const void *x)
{
	struct page *page;
	void *object = (void *)x;

	//trace_kfree(_RET_IP_,x);

	if(unlikely(ZERO_OR_NULL_PTR(x)))
		return;

	page = virt_to_head_page(x);
	/* Need debug */
//	if(unlikely(!PageSlab(page)))
	if(0)
	{
		/* Need debug */
//		BUG_ON(!PageCompound(page));
		kmemleak_free(x);
		put_page(page);
		return;
	}
	slab_free(page->slab,page,object,_RET_IP_);
}

void kmem_cache_free(struct kmem_cache *s,void *x)
{
	struct page *page;

	page = virt_to_head_page(x);

	slab_free(s,page,x,_RET_IP_);
}

static void free_kmem_cache_nodes(struct kmem_cache *s)
{
	int node;

	for_each_node_state(node,N_NORMAL_MEMORY) {
		struct kmem_cache_node *n = s->node[node];

		if(n)
			kmem_cache_free(kmem_cache_node,
					(void *)(unsigned long)__va(mem_to_phys(n))); // simulate.

		s->node[node] = NULL;
	}
}

static int init_kmem_cache_nodes(struct kmem_cache *s)
{
	int node;

	for_each_node_state(node,N_NORMAL_MEMORY) {
		struct kmem_cache_node *n;

		if(slab_state == DOWN) {
			early_kmem_cache_node_alloc(node);
			continue;
		}
		n = kmem_cache_alloc_node(kmem_cache_node,
				GFP_KERNEL,node);
		if(!n) {
			free_kmem_cache_nodes(s);
			return 0;
		}

		s->node[node] = n;
		init_kmem_cache_node(n,s);
	}
	return 1;
}

static inline int alloc_kmem_cache_cpus(struct kmem_cache *s)
{
	BUILD_BUG_ON(PERCPU_DYNAMIC_EARLY_SIZE < 
			SLUB_PAGE_SHIFT * sizeof(struct kmem_cache_cpu));
   	
	s->cpu_slab = alloc_percpu(struct kmem_cache_cpu);

	return s->cpu_slab != NULL;
}
static void set_min_partial(struct kmem_cache *s,unsigned long min)
{
	if(min < MIN_PARTIAL)
		min = MIN_PARTIAL;
	else if(min > MAX_PARTIAL)
		min = MAX_PARTIAL;
	s->min_partial = min;
}
static inline unsigned long kmem_cache_flags(unsigned long objsize,
		unsigned long flags,const char *name,
		void (*ctor)(void *))
{
	return flags;
}

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

	if(!calculate_sizes(s,-1))
		goto error;

	if(disable_higher_order_debug) {
		/*
		 * Disable debugging flags that store metadat if the min slab
		 * order increased.
		 */
		if(get_order(s->size) > get_order(s->objsize)) {
			s->flags &= ~DEBUG_METADATA_FLAGS;
			s->offset = 0;
			if(!calculate_sizes(s,-1))
				goto error;
		}
	}

	/*
	 * The larger the object size is,the more pages we want on the partial
	 * list to avoid pounding the page allocator excessively.
	 */
	set_min_partial(s,ilog2(s->size));
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
				s->name,
				(void *)(unsigned long)size,
				(void *)(unsigned long)s->size,
				(void *)(unsigned long)oo_order(s->oo),
				(void *)(unsigned long)s->offset,
				(void *)(unsigned long)flags);

	return 0;
}

/*
 * Used for early kmem_cache structures that were allocated using
 * the page allocator.
 */
static void __init kmem_cache_bootstrap_fixup(struct kmem_cache *s)
{
	int node;

	list_add(&s->list,&slab_caches);
	s->refcount = -1;

	for_each_node_state(node,N_NORMAL_MEMORY) {
		struct kmem_cache_node *n = get_node(s,node);
		struct page *p;

		if(n) {
			list_for_each_entry(p,&n->partial,lru)
				p->slab = s;
		}
	}
}
/*
 * Create kmalloc cache.
 */
static struct kmem_cache *__init create_kmalloc_cache(const char *name,
		int size,unsigned int flags)
{
	struct kmem_cache *s;

	s = kmem_cache_alloc(kmem_cache,GFP_NOWAIT);

	/*
	 * This function is called with IRQs disabled during early-boot on
	 * single CPU so there's no need to take slub_lock herer.
	 */
	if(!kmem_cache_open(s,name,size,ARCH_KMALLOC_MINALIGN,
				flags,NULL))
		goto panic;

	list_add(&s->list,&slab_caches);
	return s;

panic:
	mm_err("Creation of kmalloc slab %s size = %d failed.\n",name,size);
	return NULL;
}

void __init kmem_cache_init(void)
{
	int i;
	int caches = 0;
	struct kmem_cache *temp_kmem_cache;
	int order;
	struct kmem_cache *temp_kmem_cache_node;
	unsigned long kmalloc_size;

	kmem_size = offsetof(struct kmem_cache,node) +
		nr_node_ids * sizeof(struct kmem_cache_node *);

	/* Allocate two kmem_caches from the page allocator. */
	kmalloc_size = ALIGN(kmem_size,cache_line_size());
	order = get_order(2 * kmalloc_size);
	/* In order to use directly,so simulate... */
	kmem_cache = phys_to_mem(__pa(__get_free_pages(GFP_NOWAIT,order)));

	/*
	 * Must first have the slab cache available for the allocations of the 
	 * struct kmem_cache_node's.There is special bootstarp code in 
	 * kmem_cache_open for slab_state == DOWN
	 */
	kmem_cache_node = (void *)kmem_cache + kmalloc_size;

	kmem_cache_open(kmem_cache_node,"kmem_cache_node",
			sizeof(struct kmem_cache_node),
			0,SLAB_HWCACHE_ALIGN | SLAB_PANIC,NULL);

	/* Able to allocate the per node structures */
	slab_state = PARTIAL;

	temp_kmem_cache = kmem_cache;
	kmem_cache_open(kmem_cache,"kmem_cache",kmem_size,
			0,SLAB_HWCACHE_ALIGN | SLAB_PANIC, NULL);
	kmem_cache = kmem_cache_alloc(kmem_cache,GFP_NOWAIT);
	memcpy(kmem_cache,temp_kmem_cache,kmem_size);

	/*
	 * Allocate kmem_cache_node properly from the kmem_cache slab.
	 * kmem_cache_node is separately allocated so no need to 
	 * update any list pointers.
	 */
	temp_kmem_cache_node = kmem_cache_node;

	kmem_cache_node = kmem_cache_alloc(kmem_cache,GFP_NOWAIT);
	memcpy(kmem_cache_node,temp_kmem_cache_node,kmem_size);

	kmem_cache_bootstrap_fixup(kmem_cache_node);

	caches++;
	kmem_cache_bootstrap_fixup(kmem_cache);
	caches++;
	/* Free temporary boot structure */
	free_pages(__va(mem_to_phys(temp_kmem_cache)),order);
	
	/* Now we can use the kmem_cache to allocate kmalloc slabs */

	/*
	 * Patch up the size_index table if we have stange large alignment
	 * requirements for the kmalloc array.This is only the case for
	 * MIPS it seems.The standard arches will not generate any code here.
	 *
	 * Largest permitted alignment is 256 bytes due to the way we
	 * handle the index determination for the smaller caches.
	 *
	 * Make sure that nothing crazy happens if someone start tinkering
	 * around with ARCH_KMALLOC_MINALIGN
	 */
	BUILD_BUG_ON(KMALLOC_MIN_SIZE > 256 ||
			(KMALLOC_MIN_SIZE & (KMALLOC_MIN_SIZE - 1)));

	for(i = 8 ; i < KMALLOC_MIN_SIZE ; i+= 8) {
		int elem = size_index_elem(i);

		if(elem >= ARRAY_SIZE(size_index))
			break;
		size_index[elem] = KMALLOC_SHIFT_LOW;
	}

	if(KMALLOC_MIN_SIZE == 64) {
		/*
		 * The 96 byte size cache is not used if the alignment
		 * is 64 byte.
		 */
		for(i = 64 + 8 ; i <= 96 ; i += 8)
			size_index[size_index_elem(i)] = 7;
	} else if(KMALLOC_MIN_SIZE == 128) {
		/*
		 * The 192 byte sized cache is not used if the alignment
		 * is 128 byte.Redirect kmalloc to use the 256 byte cache
		 * instead.
		 */
		for(i = 128 + 8 ; i <= 192 ; i+= 8)
			size_index[size_index_elem(i)] = 8;
	}

	/* Caches that are not of the two-to-the-power-of size */
	if(KMALLOC_MIN_SIZE <= 32) {
		kmalloc_caches[1] = create_kmalloc_cache("kmalloc-96",96,0);
		caches++;
	}

	if(KMALLOC_MIN_SIZE <= 64) {
		kmalloc_caches[2] = create_kmalloc_cache("kmalloc-192",192,0);
		caches++;
	}

	for(i = KMALLOC_SHIFT_LOW ; i < SLUB_PAGE_SHIFT ; i++) {
		kmalloc_caches[i] = create_kmalloc_cache("kmalloc",1 << i,0);
		caches++;
	}

	slab_state = UP;

	/* Provide the correct kmalloc names now that the caches are up */
	if(KMALLOC_MIN_SIZE <= 32) {
		kmalloc_caches[1]->name = kstrdup(kmalloc_caches[1]->name,GFP_NOWAIT);
		BUG_ON(!kmalloc_caches[1]->name);
	}

	if(KMALLOC_MIN_SIZE <= 64) {
		kmalloc_caches[2]->name = kstrdup(kmalloc_caches[2]->name,GFP_NOWAIT);
		BUG_ON(!kmalloc_caches[2]->name);
	}

	for(i = KMALLOC_SHIFT_LOW ; i < SLUB_PAGE_SHIFT ; i++) {
		char *s = kmalloc_name[i - KMALLOC_SHIFT_LOW];

		/* No time to debug,so use simple way to do.. */
		BUG_ON(!s);
		kmalloc_caches[i]->name = s;
	}
#ifdef CONFIG_ZONE_DMA
	for(i = 0 ; i < SLUB_PAGE_SHIFT ; i++) {
		struct kmem_cache *s = kmalloc_caches[i];

		if(s && s->size) {
			char *name = kasprintf(GFP_NOWAIT,
					"dma-kmalloc- %d",s->objsize);

			BUG_ON(!name);
			kmalloc_dma_caches[i] = create_kmalloc_cache(name,
					s->objsize,SLUB_CACHE_DMA);
		}
	}
#endif
	mm_debug("SLUB:Genslabs=%p,HWalign=%p,Order=%p - %p,"
			"MinObjects=%p,CPUs=%p,Nodes=%p\n",
			(void *)(unsigned long)caches,
			(void *)(unsigned long)cache_line_size(),
			(void *)(unsigned long)slub_min_order,
			(void *)(unsigned long)slub_max_order,
			(void *)(unsigned long)slub_min_objects,
			(void *)(unsigned long)nr_cpu_ids,
			(void *)(unsigned long)nr_node_ids);
}

/*
 * Find a mergeable slab cache
 */
static int slab_unmergeable(struct kmem_cache *s)
{
	if(slub_nomerge || (s->flags & SLUB_NEVER_MERGE))
		return 1;

	if(s->ctor)
		return 1;

	/* 
	 * We may have set a slab to be unmergeable during bootstrap.
	 */
	if(s->refcount < 0)
		return 1;

	return 0;
}

static struct kmem_cache *find_mergeable(size_t size,
		size_t align,unsigned long flags,const char *name,
		void (*ctor)(void *))
{
	struct kmem_cache *s;

	if(slub_nomerge || (flags & SLUB_NEVER_MERGE))
		return NULL;

	if(ctor)
		return NULL;

	size = ALIGN(size,sizeof(void *));
	align = calculate_alignment(flags,align,size);
	size = ALIGN(size,align);
	flags = kmem_cache_flags(size,flags,name,NULL);

	list_for_each_entry(s,&slab_caches,list) {
		if(slab_unmergeable(s))
			continue;

		if(size > s->size)
			continue;

		if((flags & SLUB_MERGE_SAME) != (s->flags & SLUB_MERGE_SAME))
			continue;

		/*
		 * Check if alignment is compatible
		 */
		if((s->size & ~(align - 1)) != s->size)
			continue;

		if(s->size - size >= sizeof(void *))
			continue;

		return s;
	}
	return NULL;
}

static void list_slab_objects(struct kmem_cache *s,struct page *page,
		const char *text)
{
#ifdef CONFIG_SLUB_DEBUG

#else

#endif
}

/*
 * Attempt to free all partial slabs on a node
 */
static void free_partial(struct kmem_cache *s,struct kmem_cache_node *n)
{
	unsigned long flags;
	struct page *page,*h;

	spin_lock_irqsave(&n->list_lock,flags);
	list_for_each_entry_safe(page,h,&n->partial,lru) {
		if(!page->inuse) {
			__remove_partial(n,page);
			discard_slab(s,page);
		} else {
			list_slab_objects(s,page,
					"Objects remaining on kmem_cache_close()");
		}
	}
	spin_unlock_irqrestore(&n->list_lock,flags);
}

/*
 * Flush cpu slab.
 *
 * Called from IPI handler with interrupts disabled.
 */
static inline void __flush_cpu_slab(struct kmem_cache *s,int cpu)
{
	struct kmem_cache_cpu *c = per_cpu_ptr(s->cpu_slab,cpu);

	if(likely(c && c->page))
		flush_slab(s,c);
}

static void flush_cpu_slab(void *d)
{
	struct kmem_cache *s = d;

	__flush_cpu_slab(s,smp_processor_id());
}

static void flush_all(struct kmem_cache *s)
{
	on_each_cpu(flush_cpu_slab,s,1);
}

/*
 * Release all resource used by a slab cache.
 */
static inline int kmem_cache_close(struct kmem_cache *s)
{
	int node;

	flush_all(s);
	free_percpu(s->cpu_slab);
	/* Attempt to free all objects */
	for_each_node_state(node,N_NORMAL_MEMORY) {
		struct kmem_cache_node *n = get_node(s,node);

		free_partial(s,n);
		if(n->nr_partial || slabs_node(s,node))
			return 1;
	}
	free_kmem_cache_nodes(s);
	return 0;
}

void *__kmalloc_track_caller(size_t size,gfp_t gfpflags,unsigned long caller)
{
	struct kmem_cache *s;
	void *ret;

	if(unlikely(size > SLUB_MAX_SIZE))
		return kmalloc_large(size,gfpflags);

	s = get_slab(size,gfpflags);

	if(unlikely(ZERO_OR_NULL_PTR(s)))
		return s;

	ret = slab_alloc(s,gfpflags,NUMA_NO_NODE,caller);

	return ret;
}

/**** API layer *****/
struct kmem_cache *kmem_cache_create(const char *name,size_t size,
		size_t align,unsigned long flags,void (*ctor)(void *))
{
	struct kmem_cache *s;
	char *n;

	if(WARN_ON(!name))
		return NULL;

	down_write(&slub_lock);
	s = find_mergeable(size,align,flags,name,ctor);
	if(s) {
		s->refcount++;
		/*
		 * Adjust the object sizes so that we clear
		 * the complete object on kzalloc.
		 */
		s->objsize = max(s->objsize,(int)size);
		s->inuse = max_t(int,s->inuse,ALIGN(size,sizeof(void *)));

		if(sysfs_slab_alias(s,name)) {
			s->refcount--;
			goto err;
		}
		up_write(&slub_lock);
	}

	n = kstrdup(name,GFP_KERNEL);
	if(!n)
		goto err;

	s = kmalloc(kmem_size,GFP_KERNEL);
	if(s) {
		if(kmem_cache_open(s,n,size,
					align,flags,ctor)) {
			list_add(&s->list,&slab_caches);
			if(sysfs_slab_add(s)) {
				list_del(&s->list);
				kfree(n);
				kfree(s);
				goto err;
			}
			up_write(&slub_lock);
			return s;
		}
		kfree(n);
		kfree(s);
	}
err:
	up_write(&slub_lock);

	if(flags & SLAB_PANIC)
		panic("Cannot create slabcache %s\n",name);
	else
		s = NULL;
	return s;
}

/*
 * Close a cache and release the kmem_cache_structure
 * (must be used for caches created using kmem_cache_create)
 */
void kmem_cache_destroy(struct kmem_cache *s)
{
	down_write(&slab_lock);
	s->refcount--;
	if(!s->refcount) {
		list_del(&s->list);
		if(kmem_cache_close(s)) {
			mm_debug("SLUB %s: %s called for cache that "
					"still has objects.\n",s->name,__func__);
			dump_stack();
		}
		if(s->flags & SLAB_DESTROY_BY_RCU)
			rcu_barrier();
		sysfs_slab_remove(s);
	}
	up_write(&slab_lock);
}

void __init kmem_cache_init_late(void)
{
}

static int __init setup_slub_min_order(char *str)
{
//	get_option(&str,&slub_min_order);

	return 1;
}
__setup("slub_min_order",setup_slub_min_order);

static int __init setup_slub_max_order(char *str)
{
//	get_option(&str,&slub_max_order);
	slub_max_order = min(slub_max_order,MAX_ORDER  -1 );

	return 1;
}
__setup("slub_max_order",setup_slub_max_order);

static int __init setup_slub_min_objects(char *str)
{
	//get_option(&str,&slub_min_objects);

	return 1;
}
__setup("slub_min_objects",setup_slub_min_objects);

static int __init setup_slub_nomerge(char *str)
{
	slub_nomerge = 1;
	return 1;
}
__setup("slub_nomerge",setup_slub_nomerge);
