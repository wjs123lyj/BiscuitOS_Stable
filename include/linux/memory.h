#ifndef _MEMORY_H_
#define _MEMORY_H_

#ifndef arch_adjust_zones
#define arch_adjust_zones(size,holes) do {} while(0)
#endif

extern struct page *mem_map;
extern void *high_memory;

#define virt_to_page(x) (pfn_to_page(__pa(x) >> PAGE_SHIFT))
#endif
