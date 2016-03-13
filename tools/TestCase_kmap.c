/*
 * This file used to debug Kmap Allocator.
 *
 * Create by Buddy.D.Zhang
 * 2016.03.13
 */
#include "linux/kernel.h"
#include "linux/debug.h"
#include "linux/mm.h"
#include "linux/slab.h"

/*
 * TestCase_kmap
 */
void TestCase_kmap(void)
{
	struct page *page;
	unsigned int addr;

	page = alloc_page(GFP_HIGHUSER);
	if(PageHighMem(page))
		mm_debug("Page in HighMem\n");
	
	addr = kmap(page);
	mm_debug("Kmap address is %p\n",(void *)(unsigned long)addr);
}
