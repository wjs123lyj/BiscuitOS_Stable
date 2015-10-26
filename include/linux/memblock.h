#ifndef _MEMBLOCK_H_
#define _MEMBLOCK_H_
#include "list.h"
#include "page.h"
#include "types.h"
#include "boot_arch.h"
#include "kernel.h"
/*
 * This is open source,you can modify it or copy it.
 */
#define MAX_REGIONS 32
#define MAX_MEMBLOCK_TYPE 2
#define MAX_MEMBLOCK 1UL
#define INIT_MEMBLOCK_REGIONS 128

#define MM_NOREGION 22 /* No match region */
#define MM_NOEXPAND 23 /* Can't expand array */

#define RED_INACTIVE 0x09F911029D74E35BULL
#define RED_ACTIVE   0xD84156C5635688C0ULL

#define MEMBLOCK_ALLOC_ANYWHERE   (~(phys_addr_t)0)
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
	unsigned int cnt;
	unsigned int max;
	struct memblock_region *regions;
};
struct memblock {
	unsigned int current_limit;
	unsigned int memory_size;
	struct memblock_type memory;
	struct memblock_type reserved;
};

struct bootmem_data {
	unsigned int node_min_pfn;
	unsigned int node_low_pfn;
	unsigned int *node_bootmem_map;
	struct list_head list;
};

struct pglist_data {
	struct bootmem_data bdata;
	unsigned int node_id;
	unsigned int node_start_pfn;
	unsigned int node_spanned_pages;
	unsigned int node_present_pages;
	struct page *node_mem_map;
};

#define top_mem (CONFIG_BANK0_START + CONFIG_BANK0_SIZE)

extern struct memblock memblock;
extern struct list_head bdata_list;
extern struct pglist_data contig_pglist_data;
extern struct memblock_region default_memory[INIT_MEMBLOCK_REGIONS + 1];
extern struct memblock_region default_memory[INIT_MEMBLOCK_REGIONS + 1];

extern void arm_bootmem_init(unsigned int start_pfn,
		unsigned int end_pfn);
#endif
