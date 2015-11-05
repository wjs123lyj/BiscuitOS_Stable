#ifndef _BOOTMEM_H_
#define _BOOTMEM_H_
#include "list.h"

struct bootmem_data {
	unsigned long node_min_pfn;
	unsigned long node_low_pfn;
	unsigned long *node_bootmem_map;
	struct list_head list;
};

extern unsigned long max_low_pfn;
extern unsigned long min_low_pfn;
extern unsigned long max_pfn;

#endif
