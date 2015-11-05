#ifndef _HIGHMEM_H_
#define _HIGHMEM_H_

#include "pgtable.h"
/*
 * This is virtual address that used in hihgmem.
 */
#define KERNEL_RAM_VADDR   (unsigned long)0xc0008000
#define swapper_pg_dir     (unsigned long)(KERNEL_RAM_VADDR - 0x00004000)
/*
 * The virtual address of PKMAP,this is one way of mapping highmem.
 */
#define PKMAP_BASE       (unsigned long)(PAGE_OFFSET - PMD_SIZE)
#define LAST_PKMAP       PTRS_PER_PTE
#define LAST_PKMAP_MASK  (LAST_PKMAP - 1)
#define PKMAP_NR(virt)   (((virt) - PKMAP_BASE) >> PAGE_SHIFT)
#define PKMAP_ADDR(nr)   (PKMAP_BASE + ((nr) << PAGE_SHIFT))

#ifndef CONFIG_HIGHMEM
#define MAX_NR_ZONES  2
#else
#define MAX_NR_ZONES  3
#endif

extern pte_t *pkmap_page_table;
#endif
