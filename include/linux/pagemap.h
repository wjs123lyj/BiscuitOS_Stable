#ifndef _PAGEMAP_H_
#define _PAGEMAP_H_


#define page_cache_get(page)   get_page(page)
#define page_cache_release(page) put_page(page)

static inline void __set_page_locked(struct page *page)
{
	set_bit(PG_locked,&page->flags);
}

static inline void __clear_page_locked(struct page *page)
{
	clear_bit(PG_locked,&page->flags);
}

/*
 * Like add_to_page_cache_locked,but used to add newly allocated pages:
 * the page is new,so we can just run __set_page_locked() against it.
 */
static inline int add_to_page_cache(struct page *page,
		struct address_space *mapping,pgoff_t offset,gfp_t gfp_mask)
{
	int error;

	__set_page_locked(page);
	error = add_to_page_cache_locked(page,mapping,offset,gfp_mask);
	if(unlikely(error))
		__clear_page_locked(page);
	return error;
}

#endif
