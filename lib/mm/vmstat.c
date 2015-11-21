#include "../../include/linux/mmzone.h"
#include "../../include/linux/vmstat.h"
#include "../../include/linux/mm.h"

void __dec_zone_page_state(struct page *page,enum zone_stat_item item)
{
	__dec_zone_state(page_zone(page),item);
}
