#include "../../include/linux/kernel.h"
#include "../../include/linux/page.h"
#include "../../include/linux/swap.h"
#include "../../include/linux/fs.h"
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

struct address_space swapper_space;


void show_swap_cache_info(void)
{
	mm_debug("%p pages in swap cache\n",(void *)total_swapcache_pages);
	mm_debug("Swap cache state:add %p,delete %p,find %p %p\n",
			(void *)swap_cache_info.add_total,
			(void *)swap_cache_info.del_total,
			(void *)swap_cache_info.find_success,
			(void *)swap_cache_info.find_total);
	mm_debug("Free swap = %pkB\n",
			(void *)(nr_swap_pages << (PAGE_SHIFT - 10)));
	mm_debug("Total swap = %pkB\n",
			(void *)(total_swap_pages << (PAGE_SHIFT - 10)));
}
