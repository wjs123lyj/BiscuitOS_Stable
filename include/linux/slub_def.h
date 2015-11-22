#ifndef _SLUB_DEF_H_
#define _SLUB_DEF_H_

#include "list.h"
#include "atomic-long.h"
enum stat_item {
	ALLOC_FASTPATH,          /* Allocation from cpu slab */
	ALLOC_SLOWPATH,          /* Allocation by getting a new cpu slab */
	FREE_FASTPATH,           /* Free to cpu slub */
	FREE_SLOWPATH,           /* Freeing not to cpu slab */
	FREE_FROZEN,             /* Freeing to frozen slab */
	FREE_ADD_PARTIAL,        /* Freeing moves slab to partial list */
	ALLOC_FROM_PARTIAL,      /* Cpu slab acquired from partial list */
	ALLOC_SLAB,              /* Cpu slab acquried from page allocator */
	ALLOC_REFILL,            /* Refill cpu slab from slab freelist */
	FREE_SLAB,               /* Slab freed to the page allocator */
	CPUSLAB_FLUSH,           /* Abandoning of the cpu slab */
	DEACTIVATE_FULL,         /* Cpu slab was full when deactivated */
	DEACTIVATE_EMPTY,        /* Cpu slab was empty when deactivated */
	DEACTIVATE_TO_HEAD,      /* Cpu slab was moved to the head of partials */
	DEACTIVATE_TO_TAIL,      /* Cpu slab was moved to the tail of partials */
	DEACTIVATE_REMOTE_FREES, /* Slab contained remotely freed objects */
	ORDER_FALLBACK,          /* Number of times fallback was necessary */
	NR_SLUB_STAT_ITEMS
};

struct kmem_cache_cpu {
	void **freelist;    /* Pointer to first free per cpu object */
	struct page *page;  /* The slab from which we are allocating */
	int node;           /* The node of the page(or - 1 for debug)*/
#ifdef CONFIG_SLUB_STATS
	unsigned stat[NR_SLUB_STAT_ITEMS];
#endif
};
struct kmem_cache_node {
	unsigned long nr_partial;
	struct list_head partial;
#ifdef CONFIG_SLUB_DEBUG
	atomic_long_t nr_slabs;
	atomic_long_t total_objects;
	struct list_head full;
#endif
};
/*
 * Word size structure that can be atomically updated or read and that
 * contains both the order and the number of objects that a slab of the 
 * given order would contain.
 */
struct kmem_cache_order_objects {
	unsigned long x;	
};
/*
 * Slab cache management.
 */
struct kmem_cache {
	struct kmem_cache_type __percpu *cpu_slab;
	/*
	 * Userd for retriving partial slabs etc
	 */
	unsigned long flags;
	int size;     /* This size of an object including meta data */
	int objsize;  /* The size of an object without meta data */
	int offset;   /* Free pointer offset */
	struct kmem_cache_order_objects oo;

	/* Allocation and freeing of slabs */
	struct kmem_cache_order_objects max;
	struct kmem_cache_order_objects min;
	gfp_t allocflags;    /* gfp flags to use on each alloc */
	int refcount;        /* Refcount for slab cache destroy */
	void (*ctor)(void *);
	int inuse;     /* Offset to metadata */
	int align;     /* Alignment */
	unsigned long min_partial;
	const char *name;  /* Name(only for display!)*/
	struct list_head list; /* List of slab caches */
	struct kmem_cache_node *node[MAX_NUMNODES];
}

#endif
