#include "../../include/linux/kernel.h"
#include "../../include/linux/page.h"
#include "../../include/asm/errno-base.h"
#include "../../include/linux/mm_types.h"
#include "../../include/linux/pgalloc.h"

struct page *mem_map;


void *high_memory;
unsigned long num_physpages;
unsigned long totalram_pages;
unsigned long max_mapnr;
unsigned long totalhigh_pages;

extern struct mm_struct init_mm;

int __pte_alloc_kernel(pmd_t *pmd,unsigned long address)
{
	pte_t *new = pte_alloc_one_kernel(&init_mm,address);
	
	if(!new)
		return -ENOMEM;

	smp_wmb(); /* See comment in __pte_alloc */

	spin_lock(&init_mm.page_table_lock);
	if(likely(pmd_none(*pmd)))
	{
		/* Has another populated it ? */
		pmd_populate_kernel(&init_mm,pmd,new);
		new = NULL;
	} else
		VM_BUG_ON(pmd_trans_splitting(*pmd));
	spin_unlock(&init_mm.page_table_lock);
	if(new)
		pte_free_kernel(&init_mm,new);
	return 0;
}

