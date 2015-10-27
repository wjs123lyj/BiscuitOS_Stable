#ifndef _HIGHMEM_H_
#define _HIGHMEM_H_

/*
 * This is virtual address that used in hihgmem.
 */
#define KERNEL_RAM_VADDR   (unsigned long)0xc0008000
#define swapper_pg_dir     (unsigned long)(KERNEL_RAM_VADDR - 0x00004000)
#endif
