#ifndef _SWAP_H_
#define _SWAP_H_

#include "mmzone.h"


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

/* Definition of global_page_state not available yet */
#define nr_free_pages() global_page_state(NR_FREE_PAGES)

static inline int zone_reclaim(struct zone *z,gfp_t mask,unsigned int order)
{
	return 0;
}

#endif
