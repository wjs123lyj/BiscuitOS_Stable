#include "linux/kernel.h"
#include "linux/mm_types.h"
#include "linux/mmzone.h"
#include "linux/internal.h"
#include "linux/debug.h"
#include "linux/mm.h"
#include "linux/init.h"

int mminit_loglevel;

/* The zonelist are simply reported,validation is manual. */
void mminit_verify_zonelist(void)
{
	int nid;
	
	if(mminit_loglevel < MMINIT_VERIFY)
		return;

	for_each_online_node(nid) {
		pg_data_t *pgdat = NODE_DATA(nid);
		struct zone *zone;
		struct zoneref *z;
		struct zonelist *zonelist;
		int i,listid,zoneid;

		BUG_ON(MAX_ZONELISTS > 2);
		for(i = 0 ; i < MAX_ZONELISTS * MAX_NR_ZONES ; i++)	{
			
			/* Identify the zone and nodelist. */
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
			for_each_zone_zonelist(zone,z,zonelist,zoneid) {
				mm_debug("0:%s",zone->name);
			}
			mm_debug("\n");
		}
	}
}

void __meminit mminit_verify_page_links(struct page *page,enum zone_type zone,
		unsigned long nid,unsigned long pfn)
{
	BUG_ON(page_to_nid(page) != nid);
	BUG_ON(page_zonenum(page) != zone);
	BUG_ON(page_to_pfn(page) != pfn);
}

static __init int set_mminit_loglevel(char *str)
{
	get_option(&str,&mminit_loglevel);
	return 0;
}
early_param("mminit_loglevel",set_mminit_loglevel);
