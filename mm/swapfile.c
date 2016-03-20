#include "linux/kernel.h"
#include "linux/debug.h"

swap_entry_t get_swap_page(void)
{
	struct swap_info_struct *si;
	pgoff_t offset;
	int type,next;
	int wrapped = 0;

	spin_lock(&swap_lock);
	if(nr_swap_pages <= 0)
		goto noswap;
	nr_swap_pages--;

	for(type = swap_list.next ; type >= 0 && wrapped < 2 ; type = next) {
		si = swap_info[type];
		next = si->next;
		if(next < 0 ||
				(!wrapped && si->prio != swap_info[next]->prio)) {
			next = swap_list.head;
			wrapped++;
		}

		if(!si->highest_bit)
			continue;
		if(!(si->flags & SWAP_WRITEOK))
			continue;

		swap_list.next = next;
		/* This is called for allocating swap entry for cache */
		offset = scan_swap_map(si,SWAP_HASH_CACHE);
		if(offset) {
			spin_unlock(&swap_lock);
			return swap_entry(type,offset);
		}
		next = swap_list.next;
	}

	nr_swap_pages++;
noswap:
	spin_unlock(&swap_lock);
	return (swap_entry_t) {0};
}
