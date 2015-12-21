#ifndef _GFP_H_
#define _GFP_H_    1
#include "nodemask.h"
#include "mmzone.h"
#include "debug.h"
/*
 * Plain integer GFP bitmasks.Do not use this directly.
 */
#define ___GFP_DMA               0x01u
#define ___GFP_HIGHMEM           0x02u
#define ___GFP_DMA32             0x04u
#define ___GFP_MOVABLE           0x08u
#define ___GFP_WAIT              0x10u
#define ___GFP_HIGH              0x20u
#define ___GFP_IO                0x40u
#define ___GFP_FS                0x80u
#define ___GFP_COLD              0x100u
#define ___GFP_NOWARN            0x200u
#define ___GFP_REPEAT            0x400u
#define ___GFP_NOFAIL            0x800u
#define ___GFP_NORETRY           0x1000u
#define ___GFP_COMP              0x4000u
#define ___GFP_ZERO              0x8000u
#define ___GFP_NOMEMALLOC        0x10000u
#define ___GFP_HARDWALL          0x20000u
#define ___GFP_THISNODE          0x40000u
#define ___GFP_RECLAIMABLE       0x80000u
#define ___GFP_NOTRACK           0x200000u
#define ___GFP_NO_KSWAPED        0x400000u

/*
 * Action modifiers.
 */
#define __GFP_WAIT   ((gfp_t)___GFP_WAIT) /* Can wait and reschedule? */
#define __GFP_HIGH   ((gfp_t)___GFP_HIGH) /* Should access emergency pools? */
#define __GFP_IO     ((gfp_t)___GFP_IO) /* Can start physical IO? */
#define __GFP_FS     ((gfp_t)___GFP_FS) /* Can call down to low_level FS? */
#define __GFP_COLD   ((gfp_t)___GFP_COLD) /* cache-cold page required */
#define __GFP_NOWARN ((gfp_t)___GFP_NOWARN) /* Support page allocation warn */
#define __GFP_REPEAT ((gfp_t)___GFP_REPEAT) /* See above */
#define __GFP_NOFAIL ((gfp_t)___GFP_NOFAIL) /* See above */
#define __GFP_NORETRY ((gfp_t)___GFP_NORETRY) /* See above */
#define __GFP_THISNODE ((gfp_t)___GFP_THISNODE) /* No fallback,no policies */
#define __GFP_HARDWALL ((gfp_t)___GFP_HARDWALL) /* Enforce hardwall cpuset*/
#define __GFP_HIGHMEM  ((gfp_t)___GFP_HIGHMEM)
#define __GFP_RECLAIMABLE \
	                   ((gfp_t)___GFP_RECLAIMABLE) /* Page is reclaimable */

#define __GFP_DMA  ((gfp_t)___GFP_DMA)
#define __GFP_DMA32   ((gfp_t)___GFP_DMA32)
#define __GFP_MOVABLE ((gfp_t)___GFP_MOVABLE)
#define GFP_ZONEMASK  (__GFP_DMA | __GFP_HIGHMEM | __GFP_DMA32 | __GFP_MOVABLE)
#define GFP_HIGHUSER_MOVABLE (__GFP_WAIT | __GFP_IO | __GFP_FS | \
		__GFP_HARDWALL | __GFP_HIGHMEM | \
		__GFP_MOVABLE)
#define __GFP_ZERO  ((gfp_t)___GFP_ZERO)
#define __GFP_NOMEMALLOC ((gfp_t)___GFP_NOMEMALLOC)
#define __GFP_COMP  ((gfp_t)___GFP_COMP)
#define __GFP_NOTRACK ((gfp_t)___GFP_NOTRACK)

#define __GFP_NO_KSWAPD  (gfp_t)___GFP_NO_KSWAPED

/* Control allocation constraints */
#define GFP_CONSTRAINT_MASK  (__GFP_HARDWALL | __GFP_THISNODE)

/* Do not use these with a slab allocator */
#define GFP_SLAB_BUG_MASK   (__GFP_DMA32 | __GFP_HIGHMEM | ~__GFP_BITS_MASK)

#ifdef CONFIG_ZONE_DMA
#define OPT_ZONE_DMA ZONE_DMA
#else
#define OPT_ZONE_DMA ZONE_NORMAL
#endif
#ifdef CONFIG_HIGHMEM
#define OPT_ZONE_HIGHMEM ZONE_HIGHMEM
#else
#define OPT_ZONE_HIGHMEM ZONE_NORMAL
#endif
#ifdef CONFIG_ZONE_DMA32
#define OPT_ZONE_DMA32 ZONE_DMA32
#else
#define OPT_ZONE_DMA32 ZONE_NORMAL
#endif

#define GFP_THISNODE  ((gfp_t)0)

/* This equals 0,but use constants in case they ever change */
#define GFP_NOWAIT  (GFP_ATOMIC & ~__GFP_HIGH)    
#define GFP_ATOMIC  (__GFP_HIGH)
#define GFP_KERNEL  (__GFP_WAIT | __GFP_IO | __GFP_FS)

/* Control slab gfp mask during early boot */
#define GFP_BOOT_MASK (__GFP_BITS_MASK & ~(__GFP_WAIT | __GFP_IO | __GFP_FS))

/*
 * GFP_ZONE_TABLE is a word size bitstring that is used for looking up the
 * zone to use given the lowest 4 bits of gfp_t.Entries are ZONE_SHIFT long
 * and there are 16 of them to conver all poassible combinations of 
 * __GFP_DMA,__GFP_DMA32,__GFP__MOVABLE and __GFP_HIGHMEM.
 *
 * The zone fallback order is MOVABLE=>HIGHMEM=>NORMAL=>DMA32=>DMA.
 * But GFP_MOVABLE os not only a zone specifier but also an allocation
 * policy.Therefore __GFP_MOVABLE plus another zone selector is valid.
 * Only 1 bit of the lowest 3 bits(DMA,DMA32,HIGHMEM) can be set to "1".
 *
 *  bit     result
 * =================================
 *  0x0  => NORMAL 
 *  0x1  => DMA or NORMAL
 *  0x2  => HIGHMEM or NORMAL
 *  0x3  => BAD(DMA + HIGHMEM)
 *  0x4  => DMA32 or DMA or NORMAL
 *  0x5  => BAD(DMA + DMA32)
 *  0x6  => BAD(HIGHMEM + DMA32)
 *  0x7  => BAD(HIGHMEM + DMA32 + DMA)
 *  0x8  => NORMAL(MOVABLE + 0)
 *  0x9  => DMA or NORMAL(MOVABLE + DMA)
 *  0xa  => MOVABLE(Movable is valid only if HIGHMEM is set too)
 *  0xb  => BAD(MOVABLE + HIGHMEM + DMA)
 *  0xc  => DMA32(MOVABLE + HIGHMEM + DMA32)
 *  0xd  => BAD(MOVABLE + DMA32 + DMA)
 *  0xe  => BAD(MOVABLE + DMA32 + HIGHMEM)
 *  0xf  => BAD(MOVABLE + DMA32 + HIGHMEM + DMA)
 *
 * ZONE_SHIFT must be <= 2 on 32 bit platforms.
 */
#define GFP_ZONE_TABLE ( \
		(ZONE_NORMAL << 0 * ZONES_SHIFT)   \
		| (OPT_ZONE_DMA << ___GFP_DMA * ZONES_SHIFT) \
		| (OPT_ZONE_HIGHMEM << ___GFP_HIGHMEM * ZONES_SHIFT) \
		| (OPT_ZONE_DMA32 << ___GFP_MOVABLE * ZONES_SHIFT)  \
		| (ZONE_NORMAL << ___GFP_MOVABLE * ZONES_SHIFT)   \
		| (OPT_ZONE_DMA << (___GFP_MOVABLE | ___GFP_DMA) * ZONES_SHIFT)  \
		| (ZONE_MOVABLE << (___GFP_MOVABLE | ___GFP_HIGHMEM) * ZONES_SHIFT) \
		| (OPT_ZONE_DMA32 << (___GFP_MOVABLE | ___GFP_DMA32) * ZONES_SHIFT) \
		) 
/*
 * This mask makes up all the page movable related flags.
 */
#define GFP_MOVABLE_MASK (__GFP_RECLAIMABLE | __GFP_MOVABLE)
/*
 * Control page allocator reclaim behavior.
 */
#define GFP_RECLAIM_MASK (__GFP_WAIT | __GFP_HIGH | __GFP_IO | __GFP_FS | \
		__GFP_NOWARN | __GFP_REPEAT | __GFP_NOFAIL | \
		__GFP_NORETRY | __GFP_NOMEMALLOC)

#define __GFP_BITS_SHIFT    23 /* Room for 23 __GFP_FOO bits */
#define __GFP_BITS_MASK     ((gfp_t)((1 << __GFP_BITS_SHIFT) - 1))

static inline enum zone_type gfp_zone(gfp_t flags)
{
	enum zone_type z;
	int bit = (int)(flags & GFP_ZONEMASK);

	z = (GFP_ZONE_TABLE >> (bit * ZONES_SHIFT)) &
			((1 << ZONES_SHIFT) - 1);

	return z;		
}
static inline int gfp_zonelist(gfp_t flags)
{
	if(NUMA_BUILD && unlikely(flags & __GFP_THISNODE))
		return 1;
	return 0;
}
/*
 * We get the zone list from the current node and the gfp_mask.
 */
static inline struct zonelist *node_zonelist(int nid,gfp_t flags)
{
	return ((struct pglist_data *)NODE_DATA(nid))->node_zonelists + 
		gfp_zonelist(flags);
}
/*
 * Convert GFP flags to their corresponding migrate type.
 */
static inline int allocflags_to_migratetype(gfp_t gfp_flags)
{
	WARN_ON((gfp_flags & GFP_MOVABLE_MASK) == GFP_MOVABLE_MASK);

	if(unlikely(page_group_by_mobility_disabled))
		return MIGRATE_UNMOVABLE;
	/* Group based on mobility */
	return (((gfp_flags & __GFP_MOVABLE) != 0) << 1) |
		((gfp_flags & __GFP_RECLAIMABLE) != 0);
}

#define __free_page(page)  __free_pages((page),0)

extern void free_pages(unsigned long addr,unsigned int order);

#define free_page(addr) free_pages((addr),0)
#ifndef HAVE_ARCH_FREE_PAGE
static inline void arch_free_page(struct page *page,int order) {}
#endif

#define numa_node_id() 0

extern struct page *__alloc_pages_nodemask(gfp_t gfp_mask,
		unsigned int order,struct zonelist *zonelist,nodemask_t *nodemask);

static inline struct page * __alloc_pages(gfp_t gfp_mask,unsigned int order,
		struct zonelist *zonelist)
{
	return __alloc_pages_nodemask(gfp_mask,order,zonelist,NULL);
}

static inline struct page *alloc_pages_node(int nid,gfp_t gfp_mask,
		unsigned int order)
{
	/* Unknown node is current node */
	if(nid < 0)
		nid = numa_node_id();
	
	return __alloc_pages(gfp_mask,order,node_zonelist(nid,gfp_mask));
}

#define alloc_pages(gfp_mask,order)  \
	alloc_pages_node(numa_node_id(),gfp_mask,order)

#define alloc_page(gfp_mask) alloc_pages(gfp_mask,0)

#define __get_free_page(gfp_mask)   \
	__get_free_pages((gfp_mask),0)

extern gfp_t gfp_allowed_mask;


static inline struct page *alloc_pages_exact_node(int nid,gfp_t gfp_mask,
		unsigned int order)
{
	VM_BUG_ON(nid < 0 || nid >= MAX_NUMNODES);

	return __alloc_pages(gfp_mask,order,node_zonelist(nid,gfp_mask));
}

#ifndef HAVE_ARCH_ALLOC_PAGE
static inline void arch_alloc_page(struct page *page,int order) {}
#endif
#endif
