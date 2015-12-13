#ifndef _MEMORY_H_
#define _MEMORY_H_
#include "config.h"
#include "types.h"
#include "const.h"

#ifndef arch_adjust_zones
#define arch_adjust_zones(size,holes) do {} while(0)
#endif

/*
 * The highmem pkmap virtual space shares the end of the module area.
 */
#ifdef CONFIG_HIGHMEM
#define MODULES_END   (PAGE_OFFSET - PMD_SIZE)
#else
#define MODULES_END   (PAGE_OFFSET)
#endif

#ifndef CONSISTENT_DMA_SIZE
#define CONSISTENT_DMA_SIZE   SZ_2M
#endif
#define TASK_SIZE          0xC0000000
#define CONSISTENT_END     0xFFE00000UL
#define CONSISTENT_BASE    (CONSISTENT_END - CONSISTENT_DMA_SIZE)

/*
 * Priorities for the hotplug memory callback routines(stored in decreasing
 * order in the callback chain)
 */
#define SLAB_CALLBACK_PRI          1
#define IPC_CALLBACK_PRI           10


extern struct page *mem_map;
extern void *high_memory;
extern unsigned long num_physpages;
extern unsigned long totalram_pages;
extern unsigned long max_mapnr;
extern unsigned long totalhigh_pages;

#define virt_to_page(x) (pfn_to_page(__pa(x) >> PAGE_SHIFT))
/*
 * Allow for constants defined here to be used from assembly code
 * by prepending the UL suffix only with actual C code compilation
 */
#define UL(x) _AC(x,UL)
#define arch_is_coherent()      0

#define hotplug_memory_notifier(fn,pri)  do {} while(0)



#endif
