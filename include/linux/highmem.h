#ifndef _HIGHMEM_H_
#define _HIGHMEM_H_
#include "page.h"
#include "pgtable.h"
#include "kmap_types.h"
#include "uaccess.h"
#include "percpu.h"
#include "compiler.h"
#include "percpu-defs.h"
#include "fixmap.h"
/*
 * This is virtual address that used in hihgmem.
 */
#define KERNEL_RAM_VADDR   (unsigned long)0xc0008000
#define swapper_pg_dir     (unsigned long)(KERNEL_RAM_VADDR - 0x00004000)
/*
 * The virtual address of PKMAP,this is one way of mapping highmem.
 */
#define PKMAP_BASE       (unsigned long)(PAGE_OFFSET - PMD_SIZE)
#define LAST_PKMAP       PTRS_PER_PTE
#define LAST_PKMAP_MASK  (LAST_PKMAP - 1)
#define PKMAP_NR(virt)   (((virt) - PKMAP_BASE) >> PAGE_SHIFT)
#define PKMAP_ADDR(nr)   (PKMAP_BASE + ((nr) << PAGE_SHIFT))

#ifndef CONFIG_HIGHMEM
#define MAX_NR_ZONES  2
#else
#define MAX_NR_ZONES  3
#endif

#define kmap_prot    PAGE_KERNEL

DECLARE_PER_CPU(int,__kmap_atomic_idx);

static inline int kmap_atomic_idx_push(void);
void *__kmap_atomic(struct page *page)
{
	unsigned int idx;
	unsigned long vaddr;
	void *kmap;
	int type;

	pagefault_disable();
	if(!PageHighMem(page))
		return page_address(page);
#ifdef CONFIG_DEBUG_HIGHMEM
	/*
	 * There is no cache coherency issue when ono VIVT,so force the
	 * dedicated kmap usage for better debugging purposes in that case.
	 */
	if(!cache_is_vivt())
		kmap = NULL;
	else
#endif
		kmap = kmap_high_get(page);
	if(kmap)
		return kmap;

	type = kmap_atomic_idx_push();

	idx = type + KM_TYPE_NR * smp_processor_id();
	vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx);
#ifdef CONFIG_DEUBG_HIGHMEM
	/*
	 * With debugging enabled,kunmap_atomic forces that entry to 0.
	 * Make sure it was indeed properly unmapped.
	 */
	BUG_ON(!pte_none(!(TOP_PTE(vaddr))));
#endif
	set_pte_ext(TOP_PTE(vaddr),mk_pte(page,kmap_prot),0);
	/*
	 * When debugging is off,kunmap_atomic leaves the previous mapping
	 * in place,so this TLB flush ensures the TLB is updated with the 
	 * new mapping.
	 */
	local_flush_tlb_kernel_page(vaddr);

	return (void *)vaddr;
}
/*
 * Prevent people trying to call kunmap_atomic() as if it were kunmap()
 * kunmap_atomic() should get the return value of kmap_atomic,not the page.
 */
#define kunmap_atomic(addr,args...)     \
	do {     \
		BUILD_BUG_ON(__same_type((addr),struct page *));  \
		__kunmap_atomic(addr);     \
	} while(0)
/*
 * Make both:kmap_atomic(page,idx) and kmap_atomic(page) work.
 */
#define kmap_atomic(page,args...) __kmap_atomic(page)

static inline void clear_highpage(struct page *page)
{
	void *kaddr = kmap_atomic(page,KM_USER0);
	clear_page(kaddr);
	kunmap_atomic(kaddr,KM_USER0);
}
extern pte_t *pkmap_page_table;

static inline int kmap_atomic_idx_push(void)
{
	int idx = __this_cpu_inc_return(__kmap_atomic_idx) - 1;

	return idx;
}
static inline void kmap_atomic_idx_pop(void)
{
#ifdef CONFIG_DEBUG_HIGHMEM
	int idx = __this_cpu_dec_return(__kmap_atomic_idx);

	BUG_ON(idx < 0);
#else
	__this_cpu_dec(__kmap_atomic_idx);
#endif
}
#endif
