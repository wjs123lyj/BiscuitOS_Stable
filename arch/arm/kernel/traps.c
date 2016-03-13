#include "linux/kernel.h"
#include "linux/page.h"
#include "linux/debug.h"
#include "linux/pgtable.h"

void *vectors_page;

/*
 * Dump information of stack.
 */
void dump_stack(void)
{
	mm_debug("Dump!\n");
}

void __pte_error(const char *file,int line,pte_t *pte)
{
	mm_debug("%s:%d:bad pte %p\n",
			file,line,(void *)(unsigned long)pte_val(pte));
}

void __pmd_error(const char *file,int line,pmd_t *pmd)
{
	mm_debug("%s:%d:bad pmd %p\n",
			file,line,(void *)(unsigned long)pmd_val(pmd));
}

void __pgd_error(const char *file,int line,pgd_t *pgd)
{
	mm_debug("%s:%d:bad pgd %p\n",
			file,line,(void *)(unsigned long)pgd_val(pgd));
}
