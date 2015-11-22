#ifndef _SWAP_H_
#define _SWAP_H_
#include "mmzone.h"
#include "vmstat.h"

/* Definition of global_page_state not available yet */
#define nr_free_pages() global_page_state(NR_FREE_PAGES)

static inline int zone_reclaim(struct zone *z,gfp_t mask,unsigned int order)
{
	return 0;
}

#endif
