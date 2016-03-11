#ifndef _PGALLOC_H_
#define _PGALLOC_H_

#include "pgtable.h"
#include "gfp.h"
#include "pgtable-hwdef.h"
#include "domain.h"
#include "cacheflush.h"

#define _PAGE_USER_TABLE   (PMD_TYPE_TABLE | PMD_BIT4 | \
							PMD_DOMAIN(DOMAIN_USER))
#define _PAGE_KERNEL_TABLE (PMD_TYPE_TABLE | PMD_BIT4 | \
							PMD_DOMAIN(DOMAIN_KERNEL))
#define PGALLOC_GFP  (GFP_KERNEL | __GFP_NOTRACK | __GFP_REPEAT | __GFP_ZERO)
extern void *phys_to_mem(phys_addr_t addr);
/*
 * Set pmd table.
 */
static inline __pmd_populate(pmd_t *__pmd,phys_addr_t pte,
		unsigned long prot)
{
	unsigned long pmdval = (pte + PTE_HWTABLE_OFF) | prot;
	/* Simulate the L2 page table. */
	pmd_t *__ptr =
		(pmd_t *)(unsigned long)phys_to_mem(virt_to_phys((unsigned long)__pmd));
	__ptr[0] = __pmd(pmdval);
	__ptr[1] = __pmd(pmdval + 256 * sizeof(pte_t));
}
/*
 * Populate the pmdp entry with a pointer to the pte.This pmd is part
 * of the mm address space.
 */
static inline void pmd_populate_kernel(struct mm_struct *mm,pmd_t *pmdp,
		pte_t *ptep)
{
	/*
	 * The pmd must be loaded with the physical address of the PTE table.
	 */
	__pmd_populate(pmdp,__pa(ptep),_PAGE_KERNEL_TABLE);
}
static inline void clean_pte_table(pte_t *pte)
{
	clean_dcache_area(phys_to_mem(__pa((pte + PTE_HWTABLE_PTRS))),
			PTE_HWTABLE_SIZE);
}
/*
 * Allocate one PTE table.
 *
 * This actually allocates two hardware PTE tables,but we wrap this up
 * into one table thus:
 *
 * +----------------------------+
 * |  Linux pt0
 * +----------------------------+
 * |  Linux pt1
 * +----------------------------+
 * |  h/w pt0
 * +----------------------------+
 * |  h/w pt1
 * +----------------------------+
 */
static inline pte_t * pte_alloc_one_kernel(struct mm_struct *mm,
		unsigned long addr)
{
	pte_t *pte;

	pte = (pte_t *)(unsigned long)__get_free_page(PGALLOC_GFP);
	if(pte)
		clean_pte_table(pte);

	return pte;
}
/*
 * Free one PTE table.
 */
static inline void pte_free_kernel(struct mm_struct *mm,pte_t *pte)
{
	if(pte)
		free_page((unsigned long)pte);
}
#endif
