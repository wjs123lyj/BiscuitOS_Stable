#ifndef _PGTABLE_H_
#define _PGTABLE_H_

typedef struct { unsigned int pgd[2];} pgd_t;
typedef struct { unsigned int pmd;} pmd_t;
typedef struct { unsigned int pte;} pte_t;

#define PGDIR_SHIFT 21
#define PGDIR_SIZE  (1UL << PGDIR_SHIFT)
#define PGDIR_MASK  (~((PGDIR_SIZE) - 1))

#define PMD_SHIFT   21
#define PMD_SIZE    (1UL << PMD_SHIFT)

#define PTRS_PER_PGD 2048
#define PTRS_PER_PMD 1
#define PTRS_PER_PTE 512

#define PTE_HWTABLE_PTRS   (unsigned long)(PTRS_PER_PTE)
#define PTE_HWTABLE_OFF    (unsigned long)(PTE_HWTABLE_PTRS * sizeof(pte_t))
#define PTE_HWTABLE_SIZE   (unsigned long)(PTRS_PTER_PTE * 4)

#define pgd_index(addr) (unsigned long)(addr >> PGDIR_SHIFT)
#define pgd_offset(mm,addr) ((mm)->pgd + pgd_index(addr))
#define pgd_offset_k(addr) (pgd_t *)((unsigned long *)(init_mm.pgd) + pgd_index(addr))
#define pmd_offset(pgd,addr) ((pmd_t *)pgd)

/*
 * X is a value not pointer.
 */
#define pgd_val(x)  (unsigned long)(((pgd_t *)phys_to_mem(virt_to_phys(x)))->pgd[0])
#define pgd_none(x) !!!pgd_val(x)
#define pgd_bad(x)  0
#define pgd_clear(x)  do {   \
		       ((pgd_t *)phys_to_mem(virt_to_phys(x)))->pgd[0] = 0;	\
			   ((pgd_t *)phys_to_mem(virt_to_phys(x)))->pgd[1] = 0; \
		} while(0)

#define pmd_val(x)   (unsigned long)(((pmd_t *)phys_to_mem(virt_to_phys(x)))->pmd)
#define pmd_none(x)  !!!pmd_val(x)
#define pmd_bad(x)   0
#define pmd_clear(x) pgd_clear((pgd_t *)x)
#define __pmd(x) (unsigned int)(x)

#define pmd_off_k(virt) pmd_offset((pgd_t *)pgd_offset_k(virt),virt)
#define pgd_addr_end(addr,end) \
({     unsigned long __boundary = ((addr) + PGDIR_SIZE) & PGDIR_MASK; \
	   (__boundary - 1) < (end - 1) ? __boundary : end ;  \
 })
/*
 * Get the virtual address of page that pmd pointes.
 */
#define pmd_page_vaddr(pmd) (pte_t *)(__va((pmd_val(pmd) & PAGE_MASK)))
#define pte_index(x) (unsigned long)((x) >> PAGE_SHIFT)
#define pte_offset_kernel(pmd,addr) (pmd_page_vaddr(pmd) + pte_index(addr))

#define pfn_pte(pfn,prot) (unsigned int)(((pfn) << PAGE_SHIFT) | prot)

/*
 * The virtuall address of KERNEL MODULE.
 */
#define MODULES_VADDR (unsigned long)(PAGE_OFFSET - SZ_32M)
#endif
