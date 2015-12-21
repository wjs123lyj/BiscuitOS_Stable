#include "../../include/linux/kernel.h"
#include "../../include/linux/page.h"
#include "../../include/linux/debug.h"

void *vectors_page;

/*
 * Dump information of stack.
 */
void dump_stack(void)
{
	mm_debug("Dump!\n");
}

void __pgd_error(const char *file,int line,pgd_t pgd)
{
//	mm_debug("%s:%d:bad pgd %p.\n",file,line,pgd_val(pgd));
}
