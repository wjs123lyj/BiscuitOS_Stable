#ifndef _BOOTMEM_H_
#define _BOOTMEM_H_
#include "list.h"
#include "../asm/dma.h"
#include "../asm/cache.h"
#include "numa.h"

#define BOOTMEM_DEFAULT        0
#define BOOTMEM_EXCLUSIVE      (1 << 0)

typedef struct bootmem_data {
	unsigned long node_min_pfn;
	unsigned long node_low_pfn;
	unsigned long *node_bootmem_map;
	struct list_head list;
	unsigned long last_end_off;
	unsigned long hint_idx;
} bootmem_data_t;


#ifndef CONFIG_NO_BOOTMEM
extern struct bootmem_data bootmem_node_data[MAX_NUMNODES];
#endif
extern unsigned long max_low_pfn;
extern unsigned long min_low_pfn;
extern unsigned long max_pfn;

#define alloc_bootmem_node(pgdat,x) \
	__alloc_bootmem_node((pgdat),(x),SMP_CACHE_BYTES,__pa(MAX_DMA_ADDRESS))

#endif
