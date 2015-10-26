#ifndef _PGTABLE_H_
#define _PGTABLE_H_


typedef struct { unsigned int pgd[2];} pgd_t;
typedef struct { unsigned int pmd;} pmd_t;
typedef struct { unsigned int pte;} pte_t;

#define PGDIR_SHIFT 22

#define PTRS_PER_PGD 2048
#define PTRS_PER_PMD 1
#define PTRS_PER_PTE 512

#define pgd_index(addr) (addr >> PGDIR_SHIFT)
#define pgd_offset(mm,addr) ((unsigned int *)(mm)->pgd + pgd_index(addr))
#define pgd_offset_k(addr) ((unsigned int *)(init_mm.pgd) + pgd_index(addr))
#define pmd_offset(pgd,addr) ((pmd_t *)pgd)

/*
 * X is a value not pointer.
 */
#define pgd_val(x)  (((pgd_t *)phys_to_mem(virt_to_phys(x)))->pgd[0])
#define pgd_none(x) !!!pgd_val(x)
#define pgd_bad(x)  0
#define pgd_clear(x)  do {   \
		       ((pgd_t *)phys_to_mem(virt_to_phys(x)))->pgd[0] = 0;	\
			   ((pgd_t *)phys_to_mem(virt_to_phys(x)))->pgd[1] = 0; \
		} while(0)

#define pmd_val(x)   (((pmd_t *)phys_to_mem(virt_to_phys(x)))->pmd)
#define pmd_none(x)  !!!pmd_val(x)
#define pmd_bad(x)   0
#define pmd_clear(x) pgd_clear((pgd_t *)x)
#endif
