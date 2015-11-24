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
struct pglist_data *next_online_pgdat(struct pglist_data *pgdat)
{
	int nid = next_online_node(pgdata->node_id);

	if(nid == MAX_NUMNODES)
		return NULL;
	return NODE_DATA(nid);
}
struct pgdata_data *first_online_pgdat(void)
{
	return NODE_DATA(first_online_node);
}
struct pglist_data
/*
 * helper magic for for_each_zone()
 */
struct zone *next_zone(struct zone *zone)
{
	struct pglist_data *pgdat = zone->zone_pgdat;

	if(zone < pgdat->node_zones + MAX_NR_ZONES - 1)
		zone++;
	else
	{
		pgdat = next_online_pgdat(pgdat);
		if(pgdat)
			zone = pgdat->node_zones;
		else
			zone = NULL;
	}
	return zone;
}
