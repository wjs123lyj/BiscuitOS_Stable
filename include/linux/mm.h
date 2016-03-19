#ifndef _MM_H_
#define _MM_H_

#include "linux/pgtable.h"
#include "linux/mmzone.h"
#include "linux/numa.h"
#include "linux/atomic.h"
#include "linux/debug.h"
#include "linux/page-flags.h"
#include "linux/memory.h"
#include "linux/spinlock.h"


#define SECTIONS_WIDTH      0
#define ZONES_WIDTH         ZONES_SHIFT
#define NODES_WIDTH         NODES_SHIFT

#define SECTIONS_PGOFF      ((sizeof(unsigned int) *8) - SECTIONS_WIDTH)
#define NODES_PGOFF         (SECTIONS_PGOFF - NODES_WIDTH)
#define ZONES_PGOFF         (NODES_PGOFF - ZONES_WIDTH)

/*
 * Define the bit shift to access each section.For non-existant
 * sections we define the shift as 0;that plus a 0 mask ensures
 * the compiler will optimise away reference to them.
 */
#define SECTIONS_PGSHIFT   (SECTIONS_PGOFF * (SECTIONS_WIDTH != 0))
#define NODES_PGSHIFT      (NODES_PGOFF * (NODES_WIDTH != 0))
#define ZONES_PGSHIFT      (ZONES_PGOFF * (ZONES_WIDTH != 0))

/*
 * NODE:ZONE or SECTION:ZONE is used to ID a zone for the buddy allocator.
 */
#define ZONEID_SHIFT    (NODES_SHIFT + ZONES_SHIFT)
#define ZONEID_PGOFF    ((NODES_PGOFF < ZONES_PGOFF) ? \
						NODES_PGOFF : ZONES_PGOFF)
#define ZONEID_PGSHIFT  (ZONEID_PGOFF * (ZONEID_SHIFT != 0))

#define ZONES_MASK      ((1UL << ZONES_WIDTH) - 1)
#define NODES_MASK      ((1UL << NODES_WIDTH) - 1)
#define SECTIONS_MASK   ((1UL << SECTIONS_WIDTH) - 1)
#define ZONEID_MASK     ((1UL << ZONEID_SHIFT) - 1)

/*
 * Compound pages have a destructor function.Provide a 
 * prototype for that function and accessor functions.
 * These are _only_valid on the head of a PG_compound page.
 */
typedef void compound_page_dtor(struct page *);

static inline enum zone_type page_zonenum(struct page *page)
{
	return (page->flags >> ZONES_PGSHIFT) & ZONES_MASK;
}
/*
 * The identification function is only used by the buddy allocator for 
 * determining if two pages could be buddies.We are not really
 * identifying a zone since we could be using a the section number
 * id if we have not node id available in page flags.
 * We guarantee only that it will reture the same value for two
 * combinable page in zone.
 */
static inline int page_zone_id(struct page *page)
{
	return (page->flags >> ZONEID_PGSHIFT) & ZONEID_MASK;
}
static inline int zone_to_nid(struct zone *zone)
{
	return 0;
}
static inline int page_to_nid(struct page *page)
{
	return (page->flags >> NODES_PGSHIFT) & NODES_MASK;
}

static inline struct zone *page_zone(struct page *page)
{
	return &NODE_DATA(page_to_nid(page))->node_zones[page_zonenum(page)];
}

static inline pmd_t *pmd_off(pgd_t *pgd,unsigned long virt)
{
	return pmd_offset(pgd,virt);	
}

static inline pmd_t *pmd_off_k(unsigned long virt)
{
	return pmd_off(pgd_offset_k(virt),virt);
}

static inline unsigned long page_to_section(struct page *page)
{
	return (page->flags >> SECTIONS_PGSHIFT) & SECTIONS_MASK;
}
static inline void set_page_zone(struct page *page,enum zone_type zone)
{
	page->flags &= ~(ZONES_MASK << ZONES_PGSHIFT);
	page->flags |= (zone & ZONES_MASK) << ZONES_PGSHIFT;
}
static inline void set_page_node(struct page *page,unsigned long node)
{
	page->flags &= ~(NODES_MASK << NODES_PGSHIFT);
	page->flags |= (node & NODES_MASK) << NODES_PGSHIFT;
}
static inline void set_page_section(struct page *page,unsigned long section)
{
	page->flags &= ~(SECTIONS_MASK << SECTIONS_PGSHIFT);
	page->flags |= (section & SECTIONS_MASK) << SECTIONS_PGSHIFT;
}

static inline void set_page_links(struct page *page,enum zone_type zone,
		unsigned long node,unsigned long pfn)
{
	set_page_zone(page,zone);
	set_page_node(page,zone);
	set_page_section(page,pfn_to_section_nr(pfn));
}
/*
 * On an anonymous page mapped into a user virtual memory area,
 * page->mapping pointer to its anon_vma,not to a struct address_space;
 * with the PAGE_MAPPING_ANON bit set to distinguish it.
 */
#define PAGE_MAPPING_ANON   1
#define PAGE_MAPPING_KSM    2
#define PAGE_MAPPING_FLAGS  (PAGE_MAPPING_ANON | PAGE_MAPPING_KSM)
/*
 * Neutral page->mapping pointer to address_space or anon_vma or other.
 */
static inline void *page_mapping(struct page *page)
{
	return (void *)((unsigned long)page->mapping & ~PAGE_MAPPING_FLAGS);
}
static inline int PageAnon(struct page *page)
{
	return ((unsigned long)page->mapping & PAGE_MAPPING_ANON) != 0;
}
/*
 * Return the pagecache index of the passed page.Return pagecache pages
 * use->index whereas swapcache page use->private
 */
#define page_private(page)        ((page)->private)
#define set_page_private(page,v)  ((page)->private = (v))
static inline pgoff_t page_index(struct page *page)
{
	if(unlikely(PageSwapCache(page)))
		return page_private(page);
	return page->index;
}
/*
 * Methods to modify the page usage count.
 *
 * What counts for a page usage:
 * - cacahe mapping (page->mapping)
 * - private data   (page->private)
 * - page mapped in a task's page tables,each mapping
 * is counted separately.
 *
 * Also,many kernel routines increase the page count before a critical
 * routine so they can be sure the page doesn't go away from under them.
 */

/*
 * Drop a ref,return true if the refcount fell the zero(the page has no users)
 */
static inline int put_page_testzero(struct page *page)
{
	VM_BUG_ON(atomic_read(&page->_count) == 0);
	return atomic_dec_and_test(&page->_count);
}
/*
 * The atomic page->_mapcount,like _count,start from -1:
 * so that transitions both from it and to it can be tracked,
 * using atomic_inc_and_test and atomic_add_negative(-1)
 */
static inline void reset_page_mapcount(struct page *page)
{
	atomic_set(&(page)->_mapcount,-1);
}
static inline int page_mapcount(struct page *page)
{
	return atomic_read(&(page)->_mapcount) + 1;
}
/*
 * Return true if this page is mapped into pagetables.
 */
static inline int page_mapped(struct page *page)
{
	return atomic_read(&(page)->_mapcount) >= 0;
}
static inline struct page *compound_head(struct page *page)
{
	if(unlikely(PageTail(page)))
		return page->first_page;
	return page;
}
static inline int page_count(struct page *page)
{
	return atomic_read(&compound_head(page)->_count);
}
/*
 * PageBuddy() indicate that the page is free and in the buddy system.
 */
static inline int PageBuddy(struct page *page)
{
	return atomic_read(&page->_mapcount) == -2;
}
static inline int compound_order(struct page *page)
{
	if(!PageHead(page))
		return 0;
	return (unsigned long)page[1].lru.prev;
}
static inline void __ClearPageBuddy(struct page *page)
{
	VM_BUG_ON(!PageBuddy(page));
	atomic_set(&page->_mapcount,-1);
}
static inline void __SetPageBuddy(struct page *page)
{
	VM_BUG_ON(atomic_read(&page->_mapcount) != -1);
	atomic_set(&page->_mapcount,-2);
}
/*
 * Setup the page count before being freed into the page allocator for
 * the first time(boot or memory hotplug)
 */
static inline void init_page_count(struct page *page)
{
	atomic_set(&page->_count,1);
}

static inline void *lowmem_page_address(struct page *page)
{
	return (void *)(unsigned long)__va(
			PFN_PHYS(page_to_pfn(page)));
}

struct mem_type {
	pteval_t prot_pte;
	unsigned int prot_l1;
	unsigned int prot_sect;
	unsigned int domain;
};
/*
 * The upper-most page table pointer.
 */
extern pmd_t *top_pmd;

#define TOP_PTE(x)  pte_offset_kernel(top_pmd,x)


static inline void set_compound_page_dtor(struct page *page,
		compound_page_dtor *dtor)
{
	page[1].lru.next = (void *)dtor;
}
static inline struct page *virt_to_head_page(const void *x)
{
	struct page *page = (struct page *)(unsigned long)virt_to_page(x);
	return compound_head(page);
}
/*
 * Try to grab a ref unless the page has a refcount of zero,return false if 
 * that is the case.
 */
static inline int get_page_unless_zero(struct page *page)
{
	return atomic_inc_not_zero(&page->_count);
}
#define compound_lock_irqsave(x)      do {} while(0)
#define compound_unlock_irqrestore(y,x) do {} while(0)

static inline compound_page_dtor *get_compound_page_dtor(struct page *page)
{
	return (compound_page_dtor *)page[1].lru.next;
}
static inline int __pud_alloc(struct mm_struct *mm,pgd_t *pgd,
		unsigned long address)
{
	return 0;
}
static inline int __pmd_alloc(struct mm_struct *mm,pud_t *pud,
		unsigned long address)
{
	return 0;
}

static inline pud_t *pud_alloc(struct mm_struct *mm,pgd_t *pgd,
		unsigned long address)
{
	return (unlikely(pgd_none(pgd)) && __pud_alloc(mm,pgd,address)) ?
		NULL : pud_offset(pgd,address);
}
static inline pmd_t *pmd_alloc(struct mm_struct *mm,pud_t *pud,
		unsigned long address)
{
	return (unlikely(pud_none(pud)) && __pmd_alloc(mm,pud,address)) ?
		NULL : pmd_offset(pud,address);
}

/*
 * Determine if an address is within the vmalloc range.
 */
static inline int is_vmalloc_addr(const void *x)
{
#ifndef CONFIG_MMU
	unsigned long addr = (unsigned long)x;

	return addr >= VMALLOC_START && addr < VMALLOC_END;
#else
	return 0;
#endif
}
#ifdef CONFIG_DEBUG_PAGEALLOC
extern int debug_pagealloc_enabled;

extern void kernel_map_pages(struct page *page,int numpages,int enable);

static inline void enable_debug_pagealloc(void)
{
	debug_pagealloc_enabled = 1;
}
#else

#if USE_SPLIT_PTLOCKS
/*
 * We tuck a spinlock to guard each pagetable page into its struct page,
 * at page->private,with BUILD_BUG_ON to make sure that this will not
 * overflow into the next struct page(as it might with DEBUG_SPINLOCK).
 * When freeing,reset page->mapping so free_pages_check won't complain.
 */
#define __pte_lockptr(page)    &((page)->ptl)
#define pte_lock_init(_page) do {             \
			spin_lock_init(__pte_lockptr(_page));           \
} while(0)
#define pte_lock_deinit(page)    ((page)->mapping = NULL)
#define pte_lockptr(mm,pmd)      ({(void)(mm);__pte_lockptr(pmd_page(pmd));})
#else
/*
 * We use mm->page_table_lock to guard all pagetable pages of the mm.
 */
#define pte_lock_init(page)     do {} while(0)
#define pte_lock_deinit(page)   do {} while(0)
#define pte_lockptr(mm,pmd)     ({(void)(pmd);&(mm)->page_table_lock;})
#endif


static inline void enable_debug_pagealloc(void)
{
}

static inline void kernel_map_pages(struct page *page,int numpages,int enable)
{
}
#endif

#define pte_unmap_unlock(pte,ptl)      do {        \
		spin_lock(ptl);                    \
		pte_unmap(pte);                       \
} while(0)



#define pte_offset_map_lock(mm,pmd,address,ptlp)           \
	({                                              \
	     spinlock_t *__ptl = pte_lockptr(mm,pmd);          \
	     pte_t *__pte = pte_offset_map(pmd,address);        \
	     *(ptlp) = __ptl;               \
	     spin_lock(__ptl);                     \
	     __pte;                           \
	 })


#define pte_alloc_map(mm,vma,pmd,address)                        \
	((unlikely(pmd_none(pmd)) && __pte_alloc(mm,vma,     \
											 pmd,address)) ? \
	 NULL : pte_offset_map(pmd,address))

#define pte_alloc_map_lock(mm,pmd,address,ptlp)    \
	((unlikely(pmd_none(pmd)) && __pte_alloc(mm,NULL,     \
											 pmd,address)) ?  \
	 NULL : pte_offset_map_lock(mm,pmd,address,ptlp))

#define pte_alloc_kernel(pmd,address)      \
	((unlikely(pmd_none(pmd)) && __pte_alloc_kernel(pmd,address)) ? \
									NULL : pte_offset_kernel(pmd,address))

typedef int (*pte_fn_t)(pte_t *pte,pgtable_t token,unsigned long addr,
		void *data);


extern void inc_zone_page_state(struct page *page,enum zone_stat_item item);

static inline void pgtable_page_ctor(struct page *page)
{
	pte_lock_init(page);
	inc_zone_page_state(page,NR_PAGETABLE);
}

extern void dec_zone_page_state(struct page *page,enum zone_stat_item item);

static inline void pgtable_page_dtor(struct page *page)
{
	pte_lock_deinit(page);
	dec_zone_page_state(page,NR_PAGETABLE);
}

static inline void get_page(struct page *page)
{
	/*
	 * Getting a normal page or the head of compound page
	 * requires to already have an elevated page->_cound.Only if
	 * we're getting a tail page,the elevated page->_count is
	 * required only in the head page,so for tail pages the 
	 * bugcheck only verifies that the page->_count isn't
	 * negative.
	 */
	VM_BUG_ON(atomic_read(&page->_count) < !PageTail(page));
	atomic_inc(&page->_count);
	/*
	 * Getting a tail page will elevate both the head and tail
	 * page->_count(s).
	 */
	if(unlikely(PageTail(page))) {
		/*
		 * This is safe only because
		 * __split_huge_page_refcount can't run under
		 * get_page().
		 */
		VM_BUG_ON(atomic_read(&page->first_page->_count) <= 0);
		atomic_inc(&page->first_page->_count);
	}
}

#endif
