#ifndef _VMSTAT_H_
#define _VMSTAT_H_
#include <string.h>
/*
 * Initialize the vmstat.
 */
static inline void zap_zone_vm_stats(struct zone *zone)
{
	memset(zone->vm_stat,0,sizeof(zone->vm_stat));
}


#endif
