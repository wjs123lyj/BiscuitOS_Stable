#include "../../include/linux/debug.h"

static struct {
	unsigned long add_total;
	unsigned long del_total;
	unsigned long find_success;
	unsigned long find_total;
} swap_cache_info;

static unsigned int nr_swapfiles;
long nr_swap_pages;
long total_swap_pages;

void show_swap_cache_info(void)
{
	mm_debug("%p pages in swap cache\n",total_swapcache_pages);
	mm_debug("Swap cache state:add %p,delete %p,find %p %p\n",
			swap_cache_info.add_total,swap_cache_info.del_total,
			swap_cache_info.find_success,swap_cache_info.find_total);
	mm_debug("Free swap = %pkB\n",nr_swap_pages << (PAGE_SHIFT - 10));
	mm_debug("Total swap = %pkB\n",total_swap_pages << (PAGE_SHIFT - 10));
}
