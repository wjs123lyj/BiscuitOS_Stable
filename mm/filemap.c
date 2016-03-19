#include "linux/kernel.h"
#include "linux/debug.h"
#include "linux/mm_types.h"
#include "linux/fs.h"

/**
 * add_to_page_locked - add a locked page to the pagecache
 * @page: page to add
 * @mapping: the page's address_space
 * @offset: page index
 * @gfp_mask: page allocation mode
 *
 * This function is used to add a page to the pagecache.If must be locked
 * This function does not add the page to the LRU.The caller must do that.
 */
int add_to_page_cache_locked(struct page *page,struct address_space *mapping,
		pgoff_t offset,gfp_t gfp_mask)
{
	int error;

	VM_BUG_ON(!PageLocked(page));

	error = mem_cgroup_cache_charge(page,current->mm,
			gfp_mask & GFP_RECLAIM_MASK);
	if(error)
		goto out;

	error = radix_tree_preload(gfp_mask & ~__GFP_HIGHMEM);
	if(error == 0) {
		page_cache_get(page);
		page->mapping = mapping;
		page->index = offset;

		spin_lock_irq(&mapping->tree_lock);
		error = radix_tree_insert(&mapping->page_tree,offset,page);
		if(likely(!error)) {
			mapping->nrpage++;
			__inc_zone_page_state(page,NR_FILE_PAGES);
			if(PageSwapBacked(page))
				__inc_zone_page_state(page,NR_SHMEM);
			spin_unlock_irq(&mapping->tree_lock);
		} else {
			page->mapping = NULL;
			spin_unlock_irq(&mapping->tree_lock);
			mem_cgroup_uncharge_cache_page(page);
			page_cache_release(page);
		}
		radix_tree_preload_end();
	} else
		mem_cgroup_uncharge_cache_page(page);
out:
	return error;
}
