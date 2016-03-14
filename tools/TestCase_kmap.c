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
	struct page *find_page;
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;

	page = alloc_page(GFP_HIGHUSER);
	if(PageHighMem(page))
		mm_debug("Page in HighMem\n");
	
	addr = kmap(page);
	mm_debug("Kmap address is %p\n",(void *)(unsigned long)addr);

}

/*
 * TestCase_kmap_atomic
 */
void TestCase_kmap_atomic(void)
{
	struct page *page;
	unsigned long address;

	page = alloc_page(GFP_HIGHUSER);
	if(PageHighMem(page))
		mm_debug("Page in HighMem\n");

	address = (unsigned long)__kmap_atomic(page);
	mm_debug("Kmap address is %p\n",(void *)(unsigned long)address);
	__kunmap_atomic((void *)address);
}
