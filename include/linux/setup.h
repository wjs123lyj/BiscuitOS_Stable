#ifndef _SETUP_H_
#define _SETUP_H_
#include "config.h"
#include "kernel.h"
/*
 * Memory map description
 */
#ifndef CONFIG_BOTH_BANKS
#define NR_BANKS 4
#else
#define NR_BANKS 8
#endif

struct membank {
	unsigned int start;
	unsigned int size;
	unsigned int highmem;
};

struct meminfo {
	int nr_banks;
	struct membank bank[NR_BANKS];
};

extern struct meminfo meminfo;

#define for_each_bank(iter,mi) \
	for(iter = 0 ; iter < (mi)->nr_banks ; iter++)

#define bank_pfn_start(bank) phys_to_pfn((bank)->start)
#define bank_pfn_end(bank)   phys_to_pfn((bank)->start + (bank)->size)
#define bank_pfn_size(bank)  phys_to_pfn((bank)->size)
#define bank_phys_start(bank) (bank)->start
#define bank_phys_end(bank)   ((bank)->start + (bank)->size)
#define bank_phys_size(bank)  (bank)->size

extern void __init setup_arch(void);
#endif
