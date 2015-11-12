#include "../../include/linux/config.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/mmzone.h"
#include "../../include/linux/nodemask.h"
#include "../../include/linux/kernel.h"
#include <stdio.h>
#include <stdlib.h>

int page_group_by_mobility_disabled;

static inline int zref_in_nodemask(struct zoneref *zref,nodemask_t *nodes)
{
	return 1;
}
/*
 * Retruns the next zone at or below highest_zoneidx in a zonelist.
 */
struct zoneref *next_zones_zonelist(struct zoneref *z,
		enum zone_type highest_zoneidx,
		nodemask_t *nodes,struct zone **zone)
{
	/*
	 * Find the next suitable zone to use for the allocation.
	 * Only filter based on nodemask if it's set.
	 */
	if(likely(nodes == NULL))
		while(zonelist_zone_idx(z) > highest_zoneidx)
			z++;
	else
		while(zonelist_zone_idx(z) > highest_zoneidx ||
				(z->zone && ! zref_in_nodemask(z,nodes)))
			z++;
	*zone = zonelist_zone(z);
	return z;
}
