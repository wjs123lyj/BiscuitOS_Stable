#ifndef _GFP_H_
#define _GFP_H_
#include "kernel.h"
#include "mmzone.h"
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
#define __GFP_THISNODE ((gfp_t)___GFP_THISNODE) /* No fallback,no policies */
#define __GFP_HARDWALL ((gfp_t)___GFP_HARDWALL) /* Enforce hardwall cpuset*/
#define __GFP_HIGHMEM  ((gfp_t)___GFP_HIGHMEM)
#define __GFP_MOVABLE  ((gfp_t)___GFP_MOVABLE)

#define __GFP_DMA  ((gfp_t)___GFP_DMA)
#define __GFP_HIGHMEM ((gfp_t)___GFP_HIGHMEM)
#define __GFP_DMA32   ((gfp_t)___GFP_DMA32)
#define __GFP_MOVABLE ((gfp_t)___GFP_MOVABLE)
#define GFP_ZONEMASK  (__GFP_DMA | __GFP_HIGHMEM | __GFP_DMA32 | __GFP_MOVABLE)
#define GFP_HIGHUSER_MOVABLE (__GFP_WAIT | __GFP_IO | __GFP_FS | \
		__GFP_HARDWALL | __GFP_HIGHMEM | \
		__GFP_MOVABLE)

#define GFP_KERNEL  (__GFP_WAIT | __GFP_IO | __GFP_FS)

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

#define __free_page(page)  __free_pages((page),0)
#ifndef HAVE_ARCH_FREE_PAGE
static inline void arch_free_page(struct page *page,int order) {}
#endif
#endif
