#include "../../include/linux/internal.h"
#include "../../include/linux/nodemask.h"
#include "../../include/linux/mmzone.h"
#include "../../include/linux/debug.h"

int mminit_loglevel;

/*
 * The zonelist are simply reported,validation is manual.
 */
void mminit_verify_zonelist(void)
{
	int nid;

	if(mminit_loglevel < MMINIT_VERIFY)
		return;

	for_each_online_node(nid)
	{
		struct pglist_data *pgdat = NODE_DATA(nid);
		struct zone *zone;
		struct zoneref *z;
		struct zonelist *zonelist;
		int i,listid,zoneid;

		for(i = 0 ; i < MAX_ZONELISTS * MAX_NR_ZONES ; i++)
		{
			/*
			 * Identify the zone and nodelist.
			 */
			zoneid = i % MAX_NR_ZONES;
			listid = i / MAX_NR_ZONES;
			zonelist = &pgdat->node_zonelists[listid];
			zone = &pgdat->node_zones[zoneid];
			if(!populated_zone(zone))
				continue;

			/* Print information about the zonelist */
			mm_debug("mmint::zonelist %s %d:%s = ",
					listid > 0 ? "thisnode": "general",nid,
					zone->name);
			/* Iterate the zonelist */
			for_each_zone_zonelist(zone,z,zonelist,zoneid)
			{
				mm_debug("0:%s",zone->name);
			}
			mm_debug("\n");
		}
	}
}
