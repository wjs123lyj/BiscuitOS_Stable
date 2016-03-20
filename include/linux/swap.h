#ifndef _SWAP_H_
#define _SWAP_H_

#include "mmzone.h"

/*
 * A swap entry has to fit into a "unsigned long",as 
 * the entry is hidden in the "index" field of the
 * swapper address space.
 */
typedef struct {
	unsigned long val;
} swap_entry_t;

/*
 * A swap extent maps a range of a swapfile's PAGE_SIZE pages onto a range of
 * disk blocks.A list of swap extents maps the entire swapfile.(Where the 
 * term 'swapfile' refers to either a blockdevice or an IS_REG file.Apart
 * from setup,they're hadled identically.
 *
 * We always assume that blocks are of size PAGE_SIZE.
 */
struct swap_extent {
	struct list_head list;
	pgoff_t start_page;
	pgoff_t nr_pages;
	sector_t start_block;
};

/*
 * The in-memory structure used to trace swap areas.
 */
struct swap_info_struct {
	unsigned long flags;    /* SWP_USED etc:see above */
	signed short prio;    /* swap priority of this type */
	signed char type;   /* strange name for an index */
	signed char next;   /* next type on the swap list */
	unsigned int max;   /* extent of the swap_map */
	unsigned char *swap_map; /* vmalloc'ed array of usage counts */
	unsigned int lowest_bit;  /* index of first free in swap_map */
	unsigned int highest_bit; /* index of last free in swap_map */
	unsigned int pages;   /* total of usable pages of swap */
	unsigned int inuse_pages; /*number of those currently in use */
	unsigned int cluster_next;  /* likely index for next allocation */
	unsigned int cluster_nr;  /* countdown to next cluster search */
	unsigned int lowest_alloc; /* while preparing discard cluster */
	unsigned int highest_alloc; /* while preparing discard cluster */
	struct swap_extent *curr_swap_extent;
	struct swap_extent first_swap_extent;
	unsigned int old_block_size;  /* seldom refrerenced */
};

#define zone_reclaim_mode 0
/*
 * current->reclaim_state points to one of these when a task is running
 * memory reclaim.
 */
struct reclaim_state {
	unsigned long reclaimed_slab;
};

extern struct address_space swapper_space;
#define total_swapcache_pages swapper_space.nrpages

extern inline unsigned long global_page_state(enum zone_stat_item item);
/* Definition of global_page_state not available yet */
#define nr_free_pages() global_page_state(NR_FREE_PAGES)

static inline int zone_reclaim(struct zone *z,gfp_t mask,unsigned int order)
{
	return 0;
}

#endif
