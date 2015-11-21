#ifndef _PGTABLE_TYPES_H_
#define _PGTABLE_TYPES_H_
#include "kernel.h"
#include "page.h"
#include "const.h"

#define _PAGE_BIT_PRESENT           0   /* is present */
#define _PAGE_BIT_RW                1   /* writeable */
#define _PAGE_BIT_USER              2   /* userspace addressable */
#define _PAGE_BIT_PWT               3   /* page write through */
#define _PAGE_BIT_PCD               4   /* page cache disable */
#define _PAGE_BIT_ACCESSED          5   /* was accessed(raised by CPU)*/
#define _PAGE_BIT_DIRTY             6   /* was written to(raised by CPU)*/
#define _PAGE_BIT_PSE               7   /* 4 MB(or 2MB) page */
#define _PAGE_BIT_PAT               7   /* on 4KB pages */
#define _PAGE_BIT_GLOBAL            8   /* Global TLB entry PPro+ */
#define _PAGE_BIT_UNUSED1           9   /* available for programmer */
#define _PAGE_BIT_IOMAP             10  /* flag used to indicate IO mapping */
#define _PAGE_BIT_HIDDEN            11  /* hidden by kmemcheck */
#define _PAGE_BIT_PAT_LARGE         12  /* On 2MB or 1GB pages */
#define _PAGE_BIT_SPECIAL           _PAGE_BIT_UNUSED1
#define _PAGE_BIT_CPA_TEST          _PAGE_BIT_UNUSED1
#define _PAGE_BIT_SPLITTING         _PAGE_BIT_UNUSED1 
									/* Only valid on a PSE pmd*/
#define _PAGE_BIT_NX    63 /* No execute:only valid after cpuid check */

enum {
	PG_LEVEL_NONE,
	PG_LEVEL_4K,
	PG_LEVEL_2M,
	PG_LEVEL_1G,
	PG_LEVEL_NUM
};
#ifdef CONFIG_KMEMCHECK
#define _PAGE_HIDDEN    (_AT(pteval_t,1) << _PAGE_BIT_HIDDEN)
#endif
#endif
