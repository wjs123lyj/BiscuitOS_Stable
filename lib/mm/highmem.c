#include "../../include/linux/highmem.h"
#include "../../include/linux/list.h"
#include "../../include/linux/mm.h"
#include "../../include/linux/page-flags.h"
#include "../../include/linux/hash.h"

pte_t *pkmap_page_table;


#define PA_HASH_ORDER   7
/*
 * Describes on page->virtual association
 */
struct page_address_map {
	struct page *page;
	void *virtual;
	struct list_head list;
};
/*
 * Hash table bucket
 */
static struct page_address_slot {
	struct list_head lh;     /* List of page_address_maps */
//  spinlock_t lock;         /* Protect this bucket's list */
} page_address_htable[1 << PA_HASH_ORDER];

static struct page_address_slot *page_slot(struct page *page)
{
	return &page_address_htable[hash_ptr(page,PA_HASH_ORDER)];
}
/*
 * page-address - get the mapped virtual address of a page
 * Return the page's virtual address.
 */
void *page_address(struct page *page)
{
	unsigned long flags;
	void *ret;
	struct page_address_slot *pos;

	if(!PageHighMem(page))
		return lowmem_page_address(page);

	pos = page_slot(page);
	ret = NULL;
	//spin_lock_irqsave(&psa->lock,flags);
	if(!list_empty(&pos->lh))
	{
		struct page_address_map *pam;

		list_for_each_entry(pam,&pos->lh,list)
		{
			if(pam->page == page)
			{
				return pam->virtual;
				goto done;
			}
		}
	}
done:
	//spin_unlock_irqrestore(&pas->lock,flags);
	return ret;
}
