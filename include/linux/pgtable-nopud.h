#ifndef _PGTABLE_NOPUD_H_
#define _PGTABLE_NOPUD_H_

#include "page.h"
/*
 * Having the pud type consist of a pgd gets the size right,and allows
 * us to conceptually access the pgd entry that this pud is folded into
 * without casting.
 */
typedef struct {pgd_t pgd;} pud_t;

/*
 * The "pud_xxx()" functions here are trivial for a foled two-level
 * setup:the pmd is never bad,and a pmd always exists.
 */
static inline int pud_none(pud_t *pud)      { return 0;}  
static inline int pud_bad(pud_t *pud)       { return 0;}
static inline int pud_present(pud_t *pud)   { return 1;}
static inline void pud_clear(pud_t *pud)   {}

#define pud_populate(mm,pmd,pte)   do {} while(0)

static inline pud_t *pud_offset(pgd_t *pgd,unsigned long address)
{
	return (pud_t *)pgd;
}

#define pud_addr_end(addr,end)     (end)

#endif
