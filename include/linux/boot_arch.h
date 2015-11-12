#ifndef _BOOT_ARCH_H_
#define _BOOT_ARCH_H_

#include "config.h"
/*
 * Configure the number of bank that we set virtual physical memory.
 * If we define CONFIG_BOTH_BANKS,and we will use two memory bank.
 * If we don't define and we only use one memory.
 */

extern void arch_init(void);
extern struct meminfo meminfo;
extern unsigned int memory_array0[CONFIG_BANK0_SIZE / BYTE_MODIFY];
#ifdef CONFIG_BOTH_BANKS
extern unsigned int memory_array1[CONFIG_BANK1_SIZE / BYTE_MODIFY];
#endif
extern void *phys_to_mem(phys_addr_t addr);
extern phys_addr_t mem_to_phys(void *ad);
#endif
