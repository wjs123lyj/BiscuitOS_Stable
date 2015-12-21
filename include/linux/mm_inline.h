#ifndef _MM_INLINE_H_
#define _MM_INLINE_H_
#include "memcontrol.h"
#include "vmstat.h"
#include "huge_mm.h"

/*
 * should the page be on a file LRU or anon LRU?
 */
static inline int page_is_file_cache(struct page *page)
{
	/* Need debug */
//	return !PageSwapBacked(page);
	return 0;
}
/*
 * which LRU list type should a page be on?
 */
static inline enum lru_list page_lru_base_type(struct page *page)
{
	if(page_is_file_cache(page))
		return LRU_INACTIVE_FILE;
	return LRU_INACTIVE_ANON;
}
static inline void del_page_from_lru(struct zone *zone,struct page *page)
{
	enum lru_list l;

	list_del(&page->lru);
	//if(PageUnevictable(page))
	if(1)
	{
		/* Need debug */
		//__ClearPageUnevictable(page);
		l = LRU_UNEVICTABLE;
	} else
	{
		l = page_lru_base_type(page);
		/* Need debug */
		//if(PageActive(page))
		if(1)
		{
			/* Need debug */
			//__ClearPageActive(page);
			l += LRU_ACTIVE;
		}
	}
	__mod_zone_page_state(zone,NR_LRU_BASE + l ,-hpage_nr_pages(page));
	mem_cgroup_del_lru_list(page,l);
}

#endif
