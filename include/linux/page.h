#ifndef _PAGE_H_
#define _PAGE_H_

#define PAGE_SHIFT 12
#define PAGE_SIZE (unsigned int)(1UL << PAGE_SHIFT)
#define PAGE_MASK (unsigned int)(~(PAGE_SIZE - 1))

#define L1_CACHE_SHIFT 8
#define L1_CACHE_SIZE (1UL << L1_CACHE_SHIFT)
#define L1_CACHE_ALIGN L1_CACHE_SIZE
#define PAGE_ALIGN(x) (x & ~(PAGE_SIZE - 1))
#define PFN_UP(x) (((x) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(x) ((x) >> PAGE_SHIFT)

#define PAGE_OFFSET (unsigned long)0xC0000000
#define PHYS_OFFSET (unsigned long)0x50000000
#define PHYS_PFN_OFFSET (unsigned long)((PHYS_OFFSET) >> PAGE_SHIFT)


#define virt_to_phys(x) ((unsigned long)(x) - PAGE_OFFSET + PHYS_OFFSET)
#define phys_to_virt(x) ((unsigned long)(x) - PHYS_OFFSET + PAGE_OFFSET)
#define pfn_to_phys(x)  ((unsigned long)(x) << PAGE_SHIFT)
#define phys_to_pfn(x)  ((unsigned long)(x) >> PAGE_SHIFT)
#define virt_to_pfn(x)  phys_to_pfn(virt_to_phys(x))
#define pfn_to_virt(x)  phys_to_virt(pfn_to_phys(x))
#define __pa(x) virt_to_phys(x)
#define __va(x) phys_to_virt(x)
struct page {
	int id;
};
#define pfn_to_page(pfn) (mem_map + ((pfn) - PHYS_PFN_OFFSET))
#endif
