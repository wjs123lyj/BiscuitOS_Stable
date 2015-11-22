#ifndef _VMSTAT_H_
#define _VMSTAT_H_
#include <string.h>
#include "mmzone.h"
#include "atomic-long.h"
/*
 * Manage combined zone based /global counters.
 */


/*
 * vm_stat contains the global counters.
 */
atomic_long_t vm_stat[NR_VM_ZONE_STAT_ITEMS];

/*
 * Initialize the vmstat.
 */
static inline void zap_zone_vm_stats(struct zone *zone)
{
	memset(zone->vm_stat,0,sizeof(zone->vm_stat));
}
static inline void __dec_zone_state(struct zone *zone,enum zone_stat_item item)
{
	atomic_long_dec(&zone->vm_stat[item]);
	atomic_long_dec(&vm_stat[item]);
}
static inline void zone_page_state_add(long x,struct zone *zone,
		enum zone_stat_item item)
{
	atomic_long_add(x,&zone->vm_stat[item]);
	atomic_long_add(x,&vm_stat[item]);
}
/*
 * We do not maintain differentials in a single processor configuration.
 * The functions directly modify the zone and global counters.
 */
static inline void __mod_zone_page_state(struct zone *zone,
		enum zone_stat_item item,int delta)
{
	zone_page_state_add(delta,zone,item);
}
static inline unsigned long global_page_state(enum zone_stat_item item)
{
	long x = atomic_long_read(&vm_stat[item]);

	return x;
}
#endif
