#ifndef _PGTABLE_H_
#define _PGTABLE_H_
#include "pgtable_types.h"
#include "mm_types.h"
#include "pgtable-nopud.h"
#include "page.h"
#include "../asm/head.h"

#define PGDIR_SHIFT 21
#define PGDIR_SIZE  (1UL << PGDIR_SHIFT)
#define PGDIR_MASK  (~((PGDIR_SIZE) - 1))

#define PMD_SHIFT   21
#define PMD_SIZE    (1UL << PMD_SHIFT)
#define PMD_MASK    (~(PMD_SIZE - 1))


#define PTRS_PER_PGD 2048
#define PTRS_PER_PMD 1
#define PTRS_PER_PTE 512

#define PTE_HWTABLE_PTRS   (PTRS_PER_PTE)
#define PTE_HWTABLE_OFF    (PTE_HWTABLE_PTRS * sizeof(pte_t))
#define PTE_HWTABLE_SIZE   (PTRS_PER_PTE * 4)

/*
 * Linux PTE definitions.
 *
 * We keep two sets of PTEs - the hardware and the linux version.
 * This allows greater flexibility in the way we map the Linux bits
 * onto the hardware tables,and allows us to have YOUNG and DIRTY bits.
 *
 * The PTE table pointer refers to the hardware entries;the "Linux"
 * entries are stored 1024 bytes below.
 */
#define L_PTE_PRESENT        (_AT(pteval_t,1) << 0)
#define L_PTE_YOUNG          (_AT(pteval_t,1) << 1)
#define L_PTE_FILE           (_AT(pteval_t,1) << 2) /* only when !PRESENT */
#define L_PTE_DIRTY          (_AT(pteval_t,1) << 6)
#define L_PTE_RDONLY         (_AT(pteval_t,1) << 7)
#define L_PTE_USER           (_AT(pteval_t,1) << 8)
#define L_PTE_XN             (_AT(pteval_t,1) << 9)
#define L_PTE_SHARED         (_AT(pteval_t,1) << 10) /* shared(v6) */

/*
 * These are the memory types,defined to be compatible with
 * pre-ARMv6 CPUs cacheable and bufferable bits: XXCB
 */
#define L_PTE_MT_UNCACHED      (_AT(pteval_t,0x00) << 2)  /* 0000 */
#define L_PTE_MT_BUFFERABLE    (_AT(pteval_t,0x01) << 2)  /* 0001 */
#define L_PTE_MT_WRITETHROUGH  (_AT(pteval_t,0x02) << 2)  /* 0010 */
#define L_PTE_MT_WRITEBACK     (_AT(pteval_t,0x03) << 2)  /* 0011 */
#define L_PTE_MT_MINICACHE     (_AT(pteval_t,0x06) << 2)  /* 0110 */
#define L_PTE_MT_WRITEALLOC    (_AT(pteval_t,0x07) << 2)  /* 0111 */
#define L_PTE_MT_DEV_SHARED    (_AT(pteval_t,0x04) << 2)  /* 0100 */
#define L_PTE_MT_DEV_NONSHARED (_AT(pteval_t,0x0c) << 2)  /* 1100 */
#define L_PTE_MT_DEV_WC        (_AT(pteval_t,0x09) << 2)  /* 1001 */
#define L_PTE_MT_DEV_CACHED    (_AT(pteval_t,0x0b) << 2)  /* 1011 */
#define L_PTE_MT_MASK          (_AT(pteval_t,0x0f) << 2)

/*
 * The pgprot_* and protection _map entries will be fixed up in runtime
 * to include the cacheable and bufferable bits based on memory policy,
 * as well as any architecture dependent bits like global/ASID and SMP
 * shared mapping bits.
 */
#define _L_PTE_DEFAULT L_PTE_PRESENT | L_PTE_YOUNG

extern pgprot_t pgprot_user;
extern pgprot_t pgprot_kernel;

#define __PAGE_NONE        __pgprot(_L_PTE_DEFAULT | L_PTE_RDONLY | L_PTE_XN)
#define __PAGE_SHARED      __pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_XN)
#define __PAGE_SHARED_EXEC __pgprot(_L_PTE_DEFAULT | L_PTE_USER)
#define __PAGE_COPY        __pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_RDONLY \
												   | L_PTE_XN)
#define __PAGE_COPY_EXEC   __pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_RDONLY)
#define __PAGE_READONLY    __pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_RDONLY \
												   | L_PTE_XN)
#define __PAGE_READONLY_EXEC \
						   __pgprot(_L_PTE_DEFAULT | L_PTE_USER | L_PTE_RDONLY)

/*
 * The table below defines the page protection levels that we insert into our
 * Linux page table version.These get translated into the best that the 
 * architecture can perform.Note that on most ARM hardware:
 * 1) We cannot do execute protection.
 * 2) If we could do execute protection,then read is implied.
 * 3) write implies read permissions.
 */
#define __P000 __PAGE_NONE
#define __P001 __PAGE_READONLY
#define __P010 __PAGE_COPY
#define __P011 __PAGE_COPY
#define __P100 __PAGE_READONLY_EXEC
#define __P101 __PAGE_READONLY_EXEC
#define __P110 __PAGE_COPY_EXEC
#define __P111 __PAGE_COPY_EXEC

#define __S000 __PAGE_NONE
#define __S001 __PAGE_READONLY
#define __S010 __PAGE_SHARED
#define __S011 __PAGE_SHARED
#define __S100 __PAGE_READONLY_EXEC
#define __S101 __PAGE_READONLY_EXEC
#define __S110 __PAGE_SHARED_EXEC
#define __S111 __PAGE_SHARED_EXEC

extern struct mm_struct init_mm;
/* to find an entry in a page-table-directory */
#define pgd_index(addr)     ((addr) >> PGDIR_SHIFT)

#define pgd_offset(mm,addr) ((mm)->pgd + pgd_index(addr))

/* to find an entry in a kernel page-table-directory */
#define pgd_offset_k(addr)  pgd_offset(&init_mm,addr)

/* Find an entry in the second-level page table */
#define pmd_offset(pgd,addr) ((pmd_t *)pgd)

/*
 * X is a value not pointer.
 */
#define pgd_val(x)  (unsigned long)(((pgd_t *)phys_to_mem(virt_to_phys(x)))->pgd[0])
#define pgd_none(x) 0 //!!!pgd_val(x)
#define pgd_bad(x)  0
#define pgd_clear(x)  do {   \
		       ((pgd_t *)phys_to_mem(virt_to_phys(x)))->pgd[0] = 0;	\
			   ((pgd_t *)phys_to_mem(virt_to_phys(x)))->pgd[1] = 0; \
		} while(0)

#define pmd_val(x)   (unsigned long)(((pmd_t *)phys_to_mem(virt_to_phys(x)))->pmd)
#define pmd_none(x)  !pmd_val(x)
#define pmd_bad(x)   (pmd_val(x) & 2)

#define pmd_clear(pmdp)     \
	do {                      \
	    pmd_t *__pmdp = \
		(pmd_t *)(unsigned long)phys_to_mem(virt_to_phys(pmdp));  \
	    __pmdp[0] = __pmd(0);   \
		__pmdp[1] = __pmd(0);   \
	} while(0)

	


#define pgd_addr_end(addr,end) \
({     unsigned long __boundary = ((addr) + PGDIR_SIZE) & PGDIR_MASK; \
	   (__boundary - 1) < (end - 1) ? __boundary : end ;  \
 })

#define pmd_addr_end(addr,end)                   \
({   unsigned long __boundary = ((addr) + PMD_SIZE) & PMD_MASK;  \
	 (__boundary - 1 < (end) - 1) ? __boundary : (end); \
})

static inline pte_t *pmd_page_vaddr(pmd_t *pmd)
{
	return (pte_t *)__va(pmd_val(pmd) & PAGE_MASK);
}

/*
 * Get the virtual address of page that pmd pointes.
 */
#define pte_index(x) (((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

#define pte_offset_kernel(pmd,addr) (pmd_page_vaddr(pmd) + pte_index(addr))

#define pfn_pte(pfn,prot) (pte_t)(unsigned long)((unsigned long)((pfn) << \
			PAGE_SHIFT) | (unsigned long)prot)

/*
 * The virtuall address of KERNEL MODULE.
 */
#define MODULES_VADDR (unsigned long)(PAGE_OFFSET - SZ_32M)

/*
 * VMALLOC AREA
 */
extern void *high_memory;
#ifndef VMALLOC_START
#define VMALLOC_OFFSET  (8 * 1024 * 1024)
#define VMALLOC_START   (((unsigned long)high_memory + VMALLOC_OFFSET))
#endif

static inline int pte_hidden(pte_t pte)
{
	return pte_flags(pte) & _PAGE_HIDDEN;
}

#define _MODE_PROT(p,b)   __pgprot(pgprot_val(p) | (b))


#define PAGE_NONE           _MODE_PROT(pgprot_user,L_PTE_XN | L_PTE_RDONLY)
#define PAGE_SHARED         _MODE_PROT(pgprot_user,L_PTE_USER | L_PTE_XN)  
#define PAGE_SHARED_EXEC    _MODE_PROT(pgprot_user,L_PTE_USER)
#define PAGE_COPY           _MODE_PROT(pgprot_user,L_PTE_USER | \
		                               L_PTE_RDONLY | L_PTE_XN)
#define PAGE_COPY_EXEC      _MODE_PROT(pgprot_user,L_PTE_USER | L_PTE_RDONLY)
#define PAGE_READONLY       _MODE_PROT(pgprot_user,L_PTE_USER | \
									   L_PTE_RDONLY | L_PTE_XN)
#define PAGE_READONLY_EXEC  _MODE_PROT(pgprot_user,L_PTE_USER | L_PTE_RDONLY)
#define PAGE_KERNEL         _MODE_PROT(pgprot_kernel,L_PTE_XN)
#define PAGE_KERNEL_EXEC    (pgprot_t)pgprot_kernel

#define pte_set(q,w,e)   do {} while(0)
#define pte_clear(q,w,e) do {} while(0)

#define pte_pfn(pte)  (pte_val(pte) >> PAGE_SHIFT)

/*
 * Convert a physical address to a Page Frame Number and back.
 */
#define __phys_to_pfn(paddr)  ((paddr) >> PAGE_SHIFT)

#define pmd_page(pmd)   pfn_to_page(__phys_to_pfn(pnd_val(pmd)))

#define __pte_map(pmd)   (pte_t *)(unsigned long)kmap_atomic(pmd_page(*(pmd)))
#define __pte_unmap(pte) kunmap_atomic(pte)


#define pte_offset_map(pmd,addr) (__pte_map(pmd) + pte_index(addr))

#define pte_page(pte) 0 //pfn_to_page(pte_pfn(pte))
#if WAIT_FOR_DEBUG
#define mk_pte(page,prot)  (pte_t)(unsigned long)pfn_pte(page_to_pfn(page),prot)
#else
#define mk_pte(page,prot) ((pte_t)0)
#endif


static inline pte_t ptep_get_and_clear(struct mm_struct *mm,
		unsigned long address,pte_t *ptep)
{
	pte_t pte = *ptep;
	pte_clear(mm,address,ptep);
	return pte;
}

#define pte_none(pte)         0 //(!pte_val(pte))
#define pte_present(pte)      0 //(pte_val(pte) & L_PTE_PRESENT)
#define pte_write(pte)        0 //(!(pte_val(pte) & L_PTE_RDONLY))
#define pte_dirty(pte)        0 //(pte_val(pte) & L_PTE_DIRTY)
#define pte_young(pte)        0 //(pte_val(pte) & L_PTE_YOUNG)
#define pte_exec(pte)         0 //(!(pte_val(pte) & L_PTE_XN))
#define pte_special(pte)      (0)

#define pte_present_user(pte)  \
	((pte_val(pte) & (L_PTE_PRESENT | L_PTE_USER)) == \
	 (L_PTE_PRESENT | L_PTE_USER))


#define pte_unmap(pte)     __pte_unmap(pte)

/*
 * Section address mask and size difinitions.
 */
#define SECTION_SHIFT     20
#define SECTION_SIZE      (1UL << SECTION_SHIFT)
#define SECTION_MASK      (~(SECTION_SIZE - 1))

/*
 * ARMv6 supersection address mask and size definitions.
 */
#define SUPERSECTION_SHIFT   24
#define SUPERSECTION_SIZE    (1UL << SUPERSECTION_SHIFT)
#define SUPERSECTION_MASK    (~(SUPERSECTION_SIZE - 1))

extern void set_pte_ext(phys_addr_t addr,unsigned long pte,int e);

static inline void __sync_icache_dcache(pte_t pteval)
{
}

static inline void set_pte_at(struct mm_struct *mm,unsigned long addr,
		pte_t *ptep,pte_t pteval)
{
#if WAIT_FOR_DEBUG
	if(addr >= TASK_SIZE)
		set_pte_ext(ptep,pteval,0);
	else {
		__sync_icache_dcache(pteval);
		set_pte_ext(ptep,pteval,PTE_EXT_NG);
	}
#endif
}


#define pgd_none_or_clear_bad(pgd) do {} while(0)

#endif
