#ifndef _MEMORY_HOTPLUG_H_
#define _MEMORY_HOTPLUG_H_

static inline unsigned zone_span_seqbegin(struct zone *zone)
{
	return 0;
}
static inline int zone_span_seqretry(struct zone *zone,unsigned iv)
{
	return 0;
}
static inline void pgdat_resize_init(struct pglist_data *pgdat) {}

static inline void zone_seqlock_init(struct zone *zone) {}

#endif
