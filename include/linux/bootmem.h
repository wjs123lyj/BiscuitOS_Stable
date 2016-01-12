#ifndef _BOOTMEM_H_
#define _BOOTMEM_H_  1

#include "list.h"
#include "numa.h"
#include "../asm/dma.h"
#include "../asm/cache.h"

#define BOOTMEM_DEFAULT        0
#define BOOTMEM_EXCLUSIVE      (1 << 0)

/*
 * node_bootmem_map is a map pointer - the bits represent all physical
 * memory pages(including holes) on the node.
 */
typedef struct bootmem_data {
	unsigned long node_min_pfn;
	unsigned long node_low_pfn;
	void *node_bootmem_map;
	unsigned long last_end_off;
	unsigned long hint_idx;
	struct list_head list;
} bootmem_data_t;


#ifndef CONFIG_NO_BOOTMEM
extern struct bootmem_data bootmem_node_data[MAX_NUMNODES];
#endif
extern unsigned long max_low_pfn;
extern unsigned long min_low_pfn;
extern unsigned long max_pfn;

#define alloc_bootmem_node(pgdat,x) \
	__alloc_bootmem_node(pgdat,x,SMP_CACHE_BYTES,__pa(MAX_DMA_ADDRESS))

#define alloc_bootmem_low(x)  \
	__alloc_bootmem_low(x,SMP_CACHE_BYTES,0)

static inline void *alloc_remap(int nid,unsigned long size)
{
	return NULL;
}

#endif
