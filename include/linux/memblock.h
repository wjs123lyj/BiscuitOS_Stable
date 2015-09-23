#ifndef _MEMBLOCK_H_
#define _MEMBLOCK_H_
/*
 * This is open source,you can modify it or copy it.
 */
#define MAX_REGIONS 256
#define MAX_MEMBLOCK_TYPE 2
#define MAX_MEMBLOCK 1UL

/*
 * The type of memblock.
 * MEMORY - The region that we can allocate.
 * RESERVE - The region that we can't allocate.
 */
enum MEMBLOCK_TYPE {
	MEMORY,
	RESERVE,
};
/*
 * Manange an region.
 */
struct memblock_region {
	unsigned long base;
	unsigned long size;
};

struct memblock_type {
	unsigned long cnt;
	struct memblock_region regions[MAX_REGIONS];
};
struct memblock {
	struct memblock_type type[MAX_MEMBLOCK_TYPE];
};
struct memory {
	unsigned long start_pfn;
	unsigned long end_pfn;
};
/*
 * This structure uses to hold the range of
 * real memory size in pfn.
 */
struct memory memory = {
	.start_pfn = 0x50000000,
	.end_pfn   = 0x60000000,
};
struct memblock default_memblock;

static void calculate_limit(unsigned long *min,unsigned long *max_low);

#endif
