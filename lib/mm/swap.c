#include "../../include/linux/kernel.h"
#include "../../include/linux/mmzone.h"
#include "../../include/linux/mm_types.h"
#include "../../include/linux/mm.h"

/*
 * This path almost never happens for VM activity - pages are normally
 * freed via pagevecs.But it gets used by networking.
 */
static void __page_cache_release(struct page *page)
{
	if(PageLRU(page))
	{
		unsigned long flags;
		struct zone *zone = page_zone(page);

		spin_lock_irqsave(&zone->lru_lock,flags);
		VM_BUG_ON(!PageLRU(page));
		__ClearPageLRU(page);
		del_page_from_lru(zone,page);
		spin_unlock_irqrestore(&zone->lru_lock,flags);
	}
}
static void __put_single_page(struct page *page)
{
	__page_cache_release(page);
	free_hot_cold_page(page,0);
}
static void __put_compound_page(struct page *page)
{
#if 0 // Need debug
	compound_page_dtor *dtor;

	__page_cache_release(page);
	dtor = get_compound_page_dtor(page);
	(*dtor)(page);
#endif
}
static void put_compound_page(struct page *page)
{
	if(unlikely(PageTail(page)))
	{
		/* __split_huge_page_refcount can run under us */
		struct page *page_head = page->first_page;
		smp_rmd();
		/*
		 * If PageTail is still set after smp_rmb() we can be sure
		 * that the page->first_page we read wasn't a danling pointer
		 * See __split_huge_page_refcount() smp_wmb().
		 */
		if(likely(PageTail(page) && get_page_unless_zero(page_head)))
		{
			unsigned long flags;
			/*
			 * Verify that our page_head wasn't converted
			 * to a regular page before we got a reference on it.
			 */
			if(unlikely(!PageHead(page_head)))
			{
				/* PageHead is cleared after PageTail */
				smp_rmb();
				VM_BUG_ON(PageTail(page));
				goto out_put_head;
			}
		    /*
			 * Only run compound_lock on a valid PageHead.
			 * after having it pinned with
			 * get_page_unless_zero() above.
			 */
			smp_mb();
			/* page_head wasn't dangling pointer */
			flags = 0;//compound_lock_irqsave(page_head);
			if(unlikely(!PageTail(page)))
			{
				/* __split_huge_page_refcount run before us */
				compound_unlock_irqrestore(page_head,flags);
				VM_BUG_ON(PageHead(page_head));
			out_put_head:
				if(put_page_testzero(page_head))
					__put_single_page(page_head);
			out_put_single:
				if(put_page_testzero(page))
					__put_single_page(page);
				return;
			}
			VM_BUG_ON(page_head != page->first_page);
			/*
			 * We can release the refcount taken by
			 * get_page_unless_zero now that
			 * split_huge_page_refcount is blocked on the
			 * compound_lock.
			 */
			if(put_page_testzero(page_head))
				VM_BUG_ON(1);
			/* __split_huge_page_refcount will wait now */
			VM_BUG_ON(atomic_read(&page->_count) <= 0);
			atomic_dec(&page->_count);
			VM_BUG_ON(atomic_read(&page_head->_count) <= 0);
			if(put_page_testzero(page_head))
			{
				if(PageHead(page_head))
					__put_compound_page(page_head);
				else
					__put_single_page(page_head);
			}
		} else
		{
			/* page_head is a dangling pointer */
			VM_BUG_ON(PageTail(page));
			goto out_put_single;
		}
	} else if(put_page_testzero(page))
	{
		if(PageHead(page))
			__put_compound_page(page);
		else
			__put_single_page(page);
	}
}
void put_page(struct page *page)
{
	if(unlikely(PageCompound(page)))
		put_compound_page(page);
	else if(put_page_testzero(page))
		__put_single_page(page);
}
