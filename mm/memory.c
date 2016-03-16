#include "linux/kernel.h"
#include "linux/page.h"
#include "asm/errno-base.h"
#include "linux/mm_types.h"
#include "linux/pgalloc.h"
#include "linux/spinlock.h"
#include "linux/barrier.h"
#include "linux/hugetlb.h"
#include "linux/mm.h"
#include "linux/pgtable.h"

struct page *mem_map;


void *high_memory;
unsigned long num_physpages;
unsigned long totalram_pages;
unsigned long max_mapnr;
unsigned long totalhigh_pages;

extern struct mm_struct init_mm;

int __pte_alloc_kernel(pmd_t *pmd,unsigned long address)
{
	pte_t *new = pte_alloc_one_kernel(&init_mm,address);
	if(!new)
		return -ENOMEM;

	smp_wmb(); /* See comment in __pte_alloc */

	spin_lock(&init_mm.page_table_lock);
	if(likely(pmd_none(pmd))) { /* Has another populated it ? */
		pmd_populate_kernel(&init_mm,pmd,new);
		new = NULL;
	} else
		VM_BUG_ON(pmd_trans_splitting(pmd));
	spin_unlock(&init_mm.page_table_lock);
	if(new)
		pte_free_kernel(&init_mm,new);
	return 0;
}

/*
 * If a p?d_bad entry is found while walking page tables,report
 * the error,before resetting entry to p?d_none.Usually(but
 * very seldom) called out from the p?d_none_or_clear_bad macros.
 */

void pgd_clear_bad(pgd_t *pgd)
{
	pgd_ERROR(pgd);
	pgd_clear(pgd);
}

void pud_clear_bad(pud_t *pud)
{
	pud_ERROR(pud);
	pud_clear(pud);
}

void pmd_clear_bad(pmd_t *pmd)
{
	pmd_ERROR(pmd);
	pmd_clear(pmd);
}

static int apply_to_pte_range(struct mm_struct *mm,pmd_t *pmd,
		unsigned long addr,unsigned long end,
		pte_fn_t fn,void *data)
{
	pte_t *pte;
	int err;
	pgtable_t token;
	spinlock_t *ptl;

	pte = (mm == &init_mm) ?
		pte_alloc_kernel(pmd,addr) :
		pte_alloc_map_lock(mm,pmd,addr,&ptl);
	if(!pte)
		return -ENOMEM;

	BUG_ON(pmd_huge(pmd));

	arch_enter_lazy_mmu_mode();

	token = pmd_pgtable(pmd);

	do {
		err = fn(pte++,token,addr,data);
		if(err)
			break;
	} while(addr += PAGE_SIZE,addr != end);

	arch_leave_lazy_mmu_mode();

	if(mm != &init_mm)
		pte_unmap_unlock(pte - 1,ptl);
	return err;
}

static int apply_to_pmd_range(struct mm_struct *mm,pud_t *pud,
		unsigned long addr,unsigned long end,pte_fn_t fn,void *data)
{
	pmd_t *pmd;
	unsigned long next;
	int err;

	BUG_ON(pud_huge(pud));

	pmd = pmd_alloc(mm,pud,addr);
	if(!pmd)
		return -ENOMEM;

	do {
		next = pmd_addr_end(addr,end);
		err = apply_to_pte_range(mm,pmd,addr,next,fn,data);
		if(err)
			break;
	} while(pmd++ , addr = next , addr != end);
	return err;
}


static int apply_to_pud_range(struct mm_struct *mm,pgd_t *pgd,
		unsigned long addr,unsigned long end,
		pte_fn_t fn,void *data)
{
	pud_t *pud;
	unsigned long next;
	int err;

	pud = pud_alloc(mm,pgd,addr);
	if(!pud)
		return -ENOMEM;

	do {
		next = pud_addr_end(addr,end);
		err = apply_to_pmd_range(mm,pud,addr,next,fn,data);
		if(err)
			break;
	} while(pud++ , addr = next, addr != end);
	return err;
}

/*
 * Scan a region of virtual memory,filling in page tables as necessary
 * and calling a provided function on each leaf page table.
 */
int apply_to_page_range(struct mm_struct *mm,unsigned long addr,
		unsigned long size,pte_fn_t fn,void *data)
{
	pgd_t *pgd;
	unsigned long next;
	unsigned long end = addr + size;
	int err;

	BUG_ON(addr >= end);
	pgd = pgd_offset(mm,addr);
	do {
		next = pgd_addr_end(addr,end);
		err = apply_to_pud_range(mm,pgd,addr,next,fn,data);
		if(err)
			break;
	} while(pgd++,addr = next , addr != end);

	return err;
}

int __pte_alloc(struct mm_struct *mm,struct vm_area_struct *vma,
		pmd_t *pmd,unsigned long address)
{
	pgtable_t new = pte_alloc_one(mm,address);
	int wait_split_huge_page;
	
	if(!new)
		return -ENOMEM;

	/*
	 * Ensure all pte setup(eq.pte page lock and page clearing) are
	 * visible before the pte is made visible to other CPUs by being
	 * put into page tables.
	 *
	 * The other side of the story is the pointer chasing in the page
	 * table walking code(when walking the page table without locking;
	 * ie.most of the time).Fortunately,these data accesses consist
	 * of a chain of data-dependent loads,meaning most CPUs(alpha
	 * being the notable exception) will already guarantee loads are
	 * seen in-order.See the alpha page table accessors for the
	 * smp_read_barrier_depends() barriers in page table walking code.
	 */
	smp_wmb(); /* Could be smp_wmb__xxx(before|after)_spin_lock */

	spin_lock(&mm->page_table_lock);
	wait_split_huge_page = 0;
	if(likely(pmd_none(pmd))) { /* Has another populated it ? */
		mm->nr_ptes++;
		pmd_populate(mm,pmd,new);
		new = NULL;
	} else if(unlikely(pmd_trans_splitting(pmd)))
		wait_split_huge_page = 1;
	spin_unlock(&mm->page_table_lock);
	if(new)
		pte_free(mm,new);
	return 0;
}
