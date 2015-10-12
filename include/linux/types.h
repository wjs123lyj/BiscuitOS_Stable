#ifndef _TYPES_H_
#define _TYPES_H_

#include "page.h"

typedef unsigned long phys_addr_t;

#define pfn_to_phys(p) ((p) << PAGE_SHIFT)
#define phys_to_pfn(p) ((p) >> PAGE_SHIFT)
#endif
