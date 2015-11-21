#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include "atomic.h"
#include "mm_type.h"
/*
 * Memory initialisation debug and verification.
 */
enum mminit_level {
	MMINIT_WARNING,
	MMINIT_VERIFY,
	MMINIT_TRACE,
};

static inline void set_page_count(struct page *page,int v)
{
	atomic_set(&page->_count,v);
}
/*
 * Turn a non-refcounted page inot refcounted with
 * a count of one.
 */
static inline void set_page_refcounted(struct page *page)
{
	VM_BUG_ON(PageTail(page));
	VM_BUG_ON(atomic_read(&page->_count));
	set_page_count(page,1);
}
/*
 * Function for dealing with page's order in buddy system.
 * zone->lock is already acquired when we use these.
 * So,we don't need atomic page->flags operation here.
 */
static inline unsigned long page_order(struct page *page)
{
	/*
	 * PageBuddy() must be checked by the caller.
	 */
	return page_private(page);
}
#endif
