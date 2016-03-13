#include "linux/kernel.h"
#include "linux/page.h"
#include "asm/errno-base.h"
#include "linux/mm_types.h"
#include "linux/pgalloc.h"
#include "linux/spinlock.h"
#include "linux/barrier.h"

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
	if(likely(pmd_none(pmd))) { /* Has another populated it ? */
		pmd_populate_kernel(&init_mm,pmd,new);
		new = NULL;
	} else
		VM_BUG_ON(pmd_trans_splitting(pmd));
	spin_unlock(&init_mm.page_table_lock);
	if(new)
		pte_free_kernel(&init_mm,new);
	return 0;
}

/*
 * If a p?d_bad entry is found while walking page tables,report
 * the error,before resetting entry to p?d_none.Usually(but
 * very seldom) called out from the p?d_none_or_clear_bad macros.
 */

void pgd_clear_bad(pgd_t *pgd)
{
	pgd_ERROR(pgd);
	pgd_clear(pgd);
}

void pud_clear_bad(pud_t *pud)
{
	pud_ERROR(pud);
	pud_clear(pud);
}

void pmd_clear_bad(pmd_t *pmd)
{
	pmd_ERROR(pmd);
	pmd_clear(pmd);
}
