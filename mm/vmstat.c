#include "linux/kernel.h"
#include "linux/mmzone.h"
#include "linux/debug.h"
#include "linux/vmstat.h"
#include "linux/mm.h"
#include "linux/irqflags.h"

void __dec_zone_page_state(struct page *page,enum zone_stat_item item)
{
	__dec_zone_state(page_zone(page),item);
}
/*
 * Use interrupt disable to seralize counter updates.
 */
void mod_zone_page_state(struct zone *zone,enum zone_stat_item item,
		int delta)
{
	unsigned long flags;

	local_irq_save(flags);
	__mod_zone_page_state(zone,item,delta);
	local_irq_restore(flags);
}

void inc_zone_page_state(struct page *page,enum zone_stat_item item)
{
	unsigned long flags;
	struct zone *zone;

	zone = page_zone(page);
	local_irq_save(flags);
	__inc_zone_state(zone,item);
	local_irq_restore(flags);
}

void dec_zone_page_state(struct page *page,enum zone_stat_item item)
{
	unsigned long flags;

	local_irq_save(flags);
	__dec_zone_page_state(page,item);
	local_irq_restore(flags);
}
