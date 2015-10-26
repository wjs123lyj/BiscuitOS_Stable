#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "types.h"
/*
 * Configure the number of physcial memory bank.
 * If we will use two bank and we must define CONFIG_BOTH_BANKS.
 */
//#define CONFIG_BOTH_BANKS

/*
 * For this configure,we use a unsigned int array to simulate physical address.
 */
#define BYTE_MODIFY 4
/*
 * For this configure,we will set the size of virtual memory.
 */
#define VIRTUAL_MEMORY_SIZE SZ_256M
/*
 * Configure the start physical address and size of bank0.
 */
#define CONFIG_BANK0_START ((unsigned int)0x50000000)
#ifdef  CONFIG_BOTH_BANKS
#define CONFIG_BANK0_SIZE  ((unsigned int)SZ_256M)
#else
#define CONFIG_BANK0_SIZE  (unsigned int)VIRTUAL_MEMORY_SIZE
#endif
#define MAX_BANK0_PHYS_ADDR (unsigned int)(CONFIG_BANK0_START + CONFIG_BANK0_SIZE)
/*
 * Configure the start physical address and size of bank1.
 */
#ifndef CONFIG_BOTH_BANKS
#define MEMORY_HOLE_SIZE    0x000001000
#define CONFIG_BANK1_START  (unsigned int)(CONFIG_BANK0_START + \
		CONFIG_BANK0_SIZE + MEMORY_HOLE_SIZE)
#define CONFIG_BANK1_SIZE   (unsigned int)(CONFIG_MEMORY_SIZE - SZ_256M)
#define MAX_BANK1_PHYS_ADDR (unsigned int)(CONFIG_BANK1_START + CONFIG_BANK1_SIZE)
#endif
/*
 * Configure the end address of VMALLOC area.
 * We build VMALLOC area are 256M that PAGE_OFFSET,PHYS_AREA,HOLE_8M,and VMALLOC AREA.
 */
#define VMALLOC_END (void *)(PAGE_OFFSET + SZ_256M + SZ_8M + SZ_256M)
/*
 * Configure to support highmem.
 */
//#define CONFIG_HIGHMEM
#endif
