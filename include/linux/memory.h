#ifndef _MEMORY_H_
#define _MEMORY_H_

#ifndef arch_adjust_zones
#define arch_adjust_zones(size,holes) do {} while(0)
#endif

#define MODULES_VADDR (PAGE_OFFSET - 16 * 1024 * 1024)

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
#define TASK_SIZE          (UL(CONFIG_PAGE_OFFSET) - UL(0x01000000))
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



/*
 * Extern function.
 */
extern int __pte_alloc_kernel(pmd_t *pmd,unsigned long address);

/*
 * Cache flushing area - ROM
 */
#define FLUSH_BASE_PHYS   0x00000000
#define FLUSH_BASE        0xdf000000

/*
 * Conversion between a struct page and a physical address.
 *
 * Note:when converting an unknow physical address to a 
 * struct page,the resulting pointer must be validated
 * unsigned VALID_PAGE().It must return an invalid struct page
 * for any physical address not corresponding to a system
 * RAM address.
 *
 * page_to_pfn(page) convert a struct page *to a PFN number
 * pfn_to_page(pfn)  convert a _valid_PFN number to struct page *
 *
 * virt_to_page(k)   convert a _valid_virtual address to struct page *
 * virt_addr_valid(k) indicates whether a virtual address is valid
 */

#define virt_addr_valid(kaddr) ((unsigned long)(kaddr) >= PAGE_OFFSET  \
			&& (unsigned long)(kaddr) < (unsigned long)high_memory)

/*
 * Convert a page to/from a physical address.
 */
#define page_to_phys(page) (__pfn_to_phys(page_to_pfn(page)))
#define phys_to_page(phys) (pfn_to_page(__phys_to_pfn(phys)))

#endif
