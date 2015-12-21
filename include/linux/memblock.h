#ifndef _MEMBLOCK_H_
#define _MEMBLOCK_H_
/*
 * This is open source,you can modify it or copy it.
 */
#define MAX_REGIONS 32
#define MAX_MEMBLOCK_TYPE 2
#define MAX_MEMBLOCK 1UL
#define INIT_MEMBLOCK_REGIONS 128
#define MEMBLOCK_ERROR        0

#define MM_NOREGION 22 /* No match region */
#define MM_NOEXPAND 23 /* Can't expand array */

#define RED_INACTIVE 0x09F911029D74E35BULL
#define RED_ACTIVE   0xD84156C5635688C0ULL

#define MEMBLOCK_ALLOC_ANYWHERE   (~(phys_addr_t)0)
#define MEMBLOCK_ALLOC_ACCESSIBLE 0

#define for_each_memblock(memblock_type,region) \
	for(region = memblock.memblock_type.regions; \
			region < (memblock.memblock_type.regions + \
				memblock.memblock_type.cnt); \
			region++)
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
	unsigned long memory_size;
	struct memblock_type memory;
	struct memblock_type reserved;
};


#define top_mem (CONFIG_BANK0_START + CONFIG_BANK0_SIZE)

extern struct memblock memblock;
extern struct list_head bdata_list;
extern struct memblock_region default_memory[INIT_MEMBLOCK_REGIONS + 1];
extern struct memblock_region default_memory[INIT_MEMBLOCK_REGIONS + 1];

extern void arm_bootmem_init(unsigned int start_pfn,
		unsigned int end_pfn);
/*
 * pfn conversion functions.
 * 
 * While the memory MEMBLOCKs should always be page align,the reserved
 * MEMBLOCKs may not be.This accessor attempt to provide a very clear
 * idea of what they return for such non aligned MEMBLOCKs.
 */
/*
 * Return the lowest pfn interestiong with the memory region.
 */
static inline unsigned long memblock_region_memory_base_pfn(
		const struct memblock_region *reg)
{
	return PFN_UP(reg->base);
}
/*
 * Retrun the end_pfn this region.
 */
static inline unsigned long memblock_region_memory_end_pfn(
		const struct memblock_region *reg)
{
	return PFN_DOWN(reg->base + reg->size);
}
/*
 * Retrun the lowest pfn interesting with the reserved region.
 */
static inline unsigned long memblock_region_reserved_base_pfn(
		const struct memblock_region *reg)
{
	return PFN_DOWN(reg->base);
}
/*
 * Return the end_pfn this region.
 */
static inline unsigned long memblock_region_reserved_end_pfn(
		const struct memblock_region *reg)
{
	return PFN_UP(reg->base + reg->size);
}
#endif
