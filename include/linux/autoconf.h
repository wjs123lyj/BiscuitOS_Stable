#ifndef _CONFIG_H_
#define _CONFIG_H_     1

/*
 * Configure machine type
 * Machine type see mach-types.h
 */
#define CONFIG_MACH_TYPE  99
/*
 * This marco is used to ignore some code in system.
 */
#define CONFIG_NO_USE 1
/*
 * Configure the number of physcial memory bank.
 * If we will use two bank and we must define CONFIG_BOTH_BANKS.
 */
#define CONFIG_BOTH_BANKS

/*
 * For this configure,we use a unsigned int array to simulate physical address.
 */
#define BYTE_MODIFY 4
/*
 * For this configure,we will set the size of virtual memory.
 */
#define VIRTUAL_MEMORY_SIZE (unsigned long)SZ_256M
#define CONFIG_MEMORY_SIZE  (unsigned long)(SZ_1G - SZ_4M)
/*
 * Configure the start physical address and size of bank0.
 */
#define CONFIG_BANK0_START ((unsigned long)0x50000000)
#ifdef  CONFIG_BOTH_BANKS
#define CONFIG_BANK0_SIZE  ((unsigned long)SZ_256M)
#else
#define CONFIG_BANK0_SIZE  (unsigned long)VIRTUAL_MEMORY_SIZE
#endif
#define MAX_BANK0_PHYS_ADDR (unsigned long)(CONFIG_BANK0_START + CONFIG_BANK0_SIZE)
/*
 * Configure the start physical address and size of bank1.
 */
#ifdef CONFIG_BOTH_BANKS
#define MEMORY_HOLE_SIZE    (unsigned long)0x000001000
#define CONFIG_BANK1_START  (unsigned long)(CONFIG_BANK0_START + \
		CONFIG_BANK0_SIZE + MEMORY_HOLE_SIZE)
#define CONFIG_BANK1_SIZE   (unsigned long)(CONFIG_MEMORY_SIZE - SZ_256M)
#define MAX_BANK1_PHYS_ADDR (unsigned long)(CONFIG_BANK1_START + CONFIG_BANK1_SIZE)
#endif
/*
 * Configure the reserve vmalloc area.
 */
#define CONFIG_VMALLOC_RESERVE (unsigned long)(VMALLOC_END - \
		PAGE_OFFSET - SZ_8M - SZ_256M)
/*
 * Configure to support highmem.
 */
#define CONFIG_HIGHMEM
/*
 * Configure to support DMA
 */
//#define CONFIG_DMA
/*
 * Configure to support DMA32
 */
//#define CONFIG_DMA32
/*
 * Configure to support FLAT MEMORY.
 */
#define CONFIG_FLAT_NODE_MEM_MAP
/*
 * Configure to support owner for MM.
 */
#define CONFIG_MM_OWNER
/*
 * Configure the size of CACHE_L1.
 */
#define CONFIG_ARM_L1_CACHE_SHIFT 5
/*
 * Support memcheck
 */
#define CONFIG_KMEMCHECK
/*
 * Support page alloc debug.
 */
//#define CONFIG_DEBUG_PAGEALLOC
/*
 * Support page flags debug.
 */
#define CONFIG_WANT_PAGE_DEBUG_FLAGS
/*
 * Configure the address of vectors
 */
#define CONFIG_VECTORS_BASE     0xffff0000
/*
 * Configure to support MMU
 */
#define CONFIG_MMU
/*
 * Support hardware-poison
 */
#define CONFIG_MEMORY_FAILURE
/*
 * Support SLUB debug
 */
//#define CONFIG_SLUB_DEBUG
/*
 * Support the state of slub.
 */
#define CONFIG_SLUB_STATS
/*
 * Support the kmemcheck
 */
#define CONFIG_KMEMCHECK
/*
 * Supprot CMDLINE force
 */
#define CONFIG_CMDLINE_FORCE
/*
 * Support DCACHE
 */
//#define CONFIG_CPU_DCACHE_DISABLE
/*
 * Support CACACHE Writethough
 */
#define CONFIG_CPU_DCACHE_WRITETHOUGH
/*
 * Support VIVT
 */
//#define CONFIG_CPU_CACHE_VIVT
/*
 * Support VIPT
 */
#define CONFIG_CPU_CACHE_VIPT
/*
 * ARCHv7
 */
#define __LINUX_ARM_ARCH__ 7
/*
 * CMDLINE
 */
#define CONFIG_CMDLINE  \
		"console=ttySAC0,115200 root=/dev/ram init=linuxrc initrd=0x51000000,6M ramdisk_size=6144 vmalloc=256M mminit_loglevel=4"
/*
 * Configure the number of CPU.
 */
#define CONFIG_NR_CPUS 1
/*
 * Configure wether has hold in zone.
 */
#define CONFIG_HOLES_IN_ZONE 1
/*
 * Support debug VM
 */
#define CONFIG_DEBUG_VM 1
/*
 * Configure the address of PAGE_OFFSET
 */
#define CONFIG_PAGE_OFFSET 0xc0000000
/*
 * Configure to support percpu kernel address.
 */
#define CONFIG_NEED_PER_CPU_KM 1
/*
 * Support spinlock debug
 */
#define CONFIG_DEBUG_SPINLOCK 1
/*
 * Suppor CGROUP
 */
#define CONFIG_CGROUP_MEM_RES_CTLR 1
/*
 * Debug function in slub allocator
 */
#define SLUB_DEBUG_FLUSH_ALL 1
#define SLUB_DEBUG_SLAB_ORDER 1
#define SLUB_DEBUG_CALCULATE_ORDER 1
#define SLUB_DEBUG_ALLOCATE_SLAB 1
/*
 * Debug function in Buddy allocator.
 */
#define BUDDY_DEBUG_PAGE_ORDER 1
#endif
