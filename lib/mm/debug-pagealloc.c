#include "../../include/linux/page.h"
#include "../../include/linux/mm_type.h"
#include "../../include/linux/poison.h"
#include "../../include/linux/page-debug-flags.h"
#include "../../include/linux/kernel.h"
#include "../../include/linux/page-flags.h"
#include "../../include/linux/mm.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/traps.h"

extern int debug_pagealloc_enabled;

static inline set_page_poison(struct page *page)
{
	set_bit(PAGE_DEBUG_FLAG_POISON,&page->debug_flags);
}
static inline clear_page_poison(struct page *page)
{
	clear_bit(PAGE_DEBUG_FLAG_POISON,&page->debug_flags);
}
static void check_poison_mem(unsigned char *mem,size_t bytes)
{
	unsigned char *start;
	unsigned char *end;

	for(start = mem ; start < mem + bytes ; start++)
	{
		if(*start != PAGE_POISON)
			break;
	}
	if(start == mem + bytes)
		return;

	for(end = mem + bytes - 1 ; end > start ; end--)
	{
		if(*end != PAGE_POISON)
			break;
	}
	mm_debug("pagealloc:memory corruption\n");
	dump_stack();
}

static inline int page_poison(struct page *page)
{
	return test_bit(PAGE_DEBUG_FLAG_POISON,&page->debug_flags);
}

static void unpoison_highpage(struct page *page)
{
	/*
	 * See comment in poison_highapage().
	 * Highmem pages should not be poisoned for now.
	 */
	BUG_ON(page_poison(page));
}
static void poison_highpage(struct page *page)
{
	/*
	 * Page poisoning for highmem pages is not implemented.
     * 
	 * This can be called from interrupt context.
	 * So we need to create a new kmap_atomic slot for this
	 * application and it will need interrupt protection.
	 */
}
static void poison_page(struct page *page)
{
	void *addr;

	if(PageHighMem(page))
	{
		poison_highpage(page);
		return;
	}
	set_page_poison(page);
	addr = page_address(page);
	memset(addr,PAGE_POISON,PAGE_SIZE);
}
static void poison_pages(struct page *page,int n)
{
	int i;

	for(i = 0 ; i < n ; i++)
	{
		poison_page(page + i);
	}
}

static void unpoison_page(struct page *page)
{
	if(PageHighMem(page))
	{
		unpoison_highpage(page);
		return;
	}
	if(page_poison(page))
	{
		void *addr = page_address(page);

		check_poison_mem(addr,PAGE_SIZE);
		clear_page_poison(page);
	}
}

static void unpoison_pages(struct page *page,int n)
{
	int i;

	for(i = 0 ; i < n ; i++)
		unpoison_page(page + i);
}

void kernel_map_pages(struct page *page,int numpages,int enable)
{
	if(!debug_pagealloc_enabled)
		return;

	if(enable)
		unpoison_pages(page,numpages);
	else
		poison_pages(page,numpages);
}