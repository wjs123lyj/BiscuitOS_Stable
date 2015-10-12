#ifndef _MEMBLOCK_H_
#define _MEMBLOCK_H_
/*
 * This is open source,you can modify it or copy it.
 */
#define MAX_REGIONS 32
#define MAX_MEMBLOCK_TYPE 2
#define MAX_MEMBLOCK 1UL

#define MM_NOREGION 22 /* No match region */
#define MM_NOEXPAND 23 /* Can't expand array */

#define MEMBLOCK_ALLOC_ACCESSIBLE 0
#define L1_CACHE_BYTES 32

#define for_each_memblock(x,reg) \
	for(reg = x.regions; \
			reg < x.regions + x.cnt; \
			reg++)
/*
 * Manange an region.
 */
struct memblock_region {
	unsigned long base;
	unsigned long size;
};

struct memblock_type {
	unsigned long cnt;
	unsigned long max;
	struct memblock_region *regions;
};
struct memblock {
	unsigned long current_limit;
	struct memblock_type memory;
	struct memblock_type reserved;
};
struct memory {
	unsigned long start;
	unsigned long end;
};

struct bootmem_data {
	unsigned long node_min_pfn;
	unsigned long node_low_pfn;
	void *node_bootmem_map;
//	struct list_head list;
};

struct pglist_data {
	struct bootmem_data bdata;
};


#endif
