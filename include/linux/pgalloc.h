#ifndef _PGALLOC_H_
#define _PGALLOC_H_

#include "pgtable.h"
/*
 * Set pmd table.
 */
static inline __pmd_populate(pmd_t *__pmd,phys_addr_t pte,
		unsigned long prot)
{
	/* simulate the virtual memory */
	unsigned int *pmdp = phys_to_mem(virt_to_phys(__pmd));
	unsigned int pmdval = ((unsigned int)pte + PTE_HWTABLE_OFF) | prot;

	pmdp[0] = __pmd(pmdval);
	pmdp[1] = __pmd(pmdval + 256 * sizeof(pte_t));
}

#endif
