#ifndef _MEMBLOCK_H_
#define _MEMBLOCK_H_
/*
 * This is open source,you can modify it or copy it.
 */
#define MAX_REGIONS 32
#define MAX_MEMBLOCK_TYPE 2
#define MAX_MEMBLOCK 1UL

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
	unsigned long limit;
	struct memblock_region *regions;
};
struct memblock {
	struct memblock_type memory;
	struct memblock_type reserved;
};
struct memory {
	unsigned long start_pfn;
	unsigned long end_pfn;
};

#endif
