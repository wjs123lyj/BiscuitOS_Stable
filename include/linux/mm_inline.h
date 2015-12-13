#ifndef _MM_INLINE_H_
#define _MM_INLINE_H_


/*
 * should the page be on a file LRU or anon LRU?
 */
static inline int page_is_file_cache(struct page *page)
{
	return !PageSwapBacked(page);
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
	if(PageUnevictable(page))
	{
		__ClearPageUnevictable(page);
		l = LRU_UNEVICTABLE;
	} else
	{
		l = page_lru_base_type(page);
		if(PageActive(page))
		{
			__ClearPageActive(page);
			l += LRU_ACTIVE;
		}
	}
	__mod_zone_page_state(zone,NR_LRU_BASE + l ,-hpage_nr_pages(page));
	mem_cgroup_del_lru_list(page,l);
}

#endif
