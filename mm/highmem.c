#include "linux/kernel.h"
#include "linux/kmap_types.h"
#include "linux/fixmap.h"
#include "linux/page.h"
#include "linux/list.h"
#include "linux/highmem.h"
#include "linux/mm.h"
#include "linux/hash.h"
#include "linux/spinlock.h"
#include "linux/cachetype.h"
#include "linux/uaccess.h"
#include "linux/tlbflush.h"
#include "linux/hardirq.h"

pte_t *pkmap_page_table;

static int pkmap_count[LAST_PKMAP];
int __kmp_atomic_idx;
static unsigned int last_pkmap_nr;

#define PA_HASH_ORDER   7
/*
 * Describes on page->virtual association
 */
struct page_address_map {
	struct page *page;
	void *virtual;
	struct list_head list;
};
/*
 * page_address_map freelist,allocated from page_address_maps.
 */
static struct list_head page_address_pool;  /* freelist */
static spinlock_t pool_lock;    /* protect page_address_pool */
/*
 * Hash table bucket
 */
static struct page_address_slot {
	struct list_head lh;     /* List of page_address_maps */
    spinlock_t lock;         /* Protect this bucket's list */
} page_address_htable[1 << PA_HASH_ORDER];

static DEFINE_SPINLOCK(kmap_lock);

#define lock_kmap()    spin_lock(&kmap_lock)
#define unlock_kmap()  spin_unlock(&kmap_lock)
#define unlock_kmap_any(flags) do {} while(0)
#define lock_kmap_any(flags) do {} while(0)

static struct page_address_slot *page_slot(struct page *page)
{
	return &page_address_htable[hash_ptr(page,PA_HASH_ORDER)];
}

/**
 * page_address - get the mapped virtual address of a page
 * @page:&struct page to get the virtual address of
 *
 * Return the page's virtual address.
 */
void *page_address(struct page *page)
{
	unsigned long flags;
	void *ret;
	struct page_address_slot *pos;


	if(!PageHighMem(page)) 
		return lowmem_page_address(page);

	pos = page_slot(page);
	ret = NULL;
	spin_lock_irqsave(&psa->lock,flags);
	if(!list_empty(&pos->lh)) {
		struct page_address_map *pam;

		list_for_each_entry(pam,&pos->lh,list) {
			if(pam->page == page) {
				return pam->virtual;
				goto done;
			}
		}
	}
done:
	spin_unlock_irqrestore(&pas->lock,flags);
	return ret;
}

void kunmap_high(struct page *page);

void __kunmap_atomic(void *kvaddr)
{
	unsigned long vaddr = (unsigned long)kvaddr & PAGE_MASK;
	int idx,type;

	if(kvaddr >= (void *)FIXADDR_START) {
		type = kmap_atomic_idx();
		idx  = type + KM_TYPE_NR * smp_processor_id();

		if(cache_is_vivt())
			; //__cpuc_flush_dcache_area((void *)vaddr,PAGE_SIZE);
#ifdef CONFIG_DEBUG_HIGHMEM
		set_pte_ext(TOP_PTE(vaddr),__pte(0),0);
		local_flush_tlb_kernel_page(vaddr);
#else
		(void)idx;  /* to kill a warning */
#endif
		kmap_atomic_idx_pop();
	} else if(vaddr >= PKMAP_ADDR(0) && vaddr < PKMAP_ADDR(LAST_PKMAP)) {
		/* This address was obtained through kmap_high_get() */
			kunmap_high(pte_page(&pkmap_page_table[PKMAP_NR(vaddr)]));
	}
	pagefault_enable();
}
static struct page_address_map page_address_maps[LAST_PKMAP];

void __init page_address_init(void)
{
	int i;

	INIT_LIST_HEAD(&page_address_pool);
	for(i = 0 ; i < ARRAY_SIZE(page_address_maps); i++)
		list_add(&page_address_maps[i].list,&page_address_pool);
	for(i = 0 ; i < ARRAY_SIZE(page_address_htable); i ++) {
		INIT_LIST_HEAD(&page_address_htable[i].lh);
		spin_lock_init(&page_address_htable[i].lock);
	}
	spin_lock_init(&pool_lock);
}

/**
 * kmap_high_get - pin a highmem page into memory
 * @page: &struct page to pfn
 *
 * Returns the page's current virtual memory address,or NULL if no mapping
 * exists.If and only if a non null address is returned then a 
 * matching call to kunmap_high() is necessary.
 *
 * This can be called from any context.
 */
void *kmap_high_get(struct page *page)
{
	unsigned long vaddr,flags;
	
	lock_kmap_any(flags);
	vaddr = (unsigned long)page_address(page);
	if(vaddr) {
		BUG_ON(pkmap_count[PKMAP_NR(vaddr)] < 1);
		pkmap_count[PKMAP_NR(vaddr)]++;
	}
	unlock_kmap_any(flags);
	return (void *)vaddr;
}

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
	if(!cache_is_vivt())
		kmap = NULL;
	else
#endif
		kmap = kmap_high_get(page);
	if(kmap)
		return kmap;

	type = kmap_atomic_idx_push();

	idx = type + KM_TYPE_NR * 1;
	vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx);
#ifdef CONFIG_DEBUG_HIGHMEM
	/*
	 * With debugging enabled,kunmap_atomic forces that entry to 0.
	 * Make sure it was indeed properly unmapped.
	 */
	BUG_ON(!pte_none((TOP_PTE(vaddr))));
#endif
	set_pte_ext(TOP_PTE(vaddr),mk_pte(page,kmap_prot),0);
	/*
	 * When debugging is off,kunmap_atomic leaves the previous mapping
	 * in place,so this TLB flush ensure the TLB is update with the
	 * new mapping.
	 */

	return (void *)vaddr;
}

/**
 * set_page_address - set a page's virtual address
 * @page: &struct page to set
 * @virtual:virtual address to use
 */
void set_page_address(struct page *page,void *virtual)
{
	unsigned long flags;
	struct page_address_slot *pas;
	struct page_address_map *pam;

	BUG_ON(!PageHighMem(page));

	pas = page_slot(page);
	if(virtual) {  /* Add */
		BUG_ON(list_empty(&page_address_pool));

		spin_lock_irqsave(&pool_lock,flags);
		pam = list_entry(page_address_pool.next,
				struct page_address_map,list);
		list_del(&pam->list);
		spin_unlock_irqrestore(&pool_lock,flags);

		pam->page = page;
		pam->virtual = virtual;

		spin_lock_irqsave(&pas->lock,flags);
		list_add_tail(&pam->list,&pas->lh);
		spin_unlock_irqrestore(&pas->lock,flags);
	} else {
		spin_lock_irqsave(&pas->lock,flags);
		list_for_each_entry(pam,&pas->lh,list) {
			if(pam->page = page) {
				list_del(&pam->list);
				spin_unlock_irqrestore(&pas->lock,flags);
				spin_lock_irqsave(&pool_lock,flags);
				list_add_tail(&pam->list,&page_address_pool);
				spin_unlock_irqrestore(&pool_lock,flags);
				goto done;
			}
		}
		spin_unlock_irqrestore(&pas->lock,flags);
	}
done:
	return;
}

static void flush_all_zero_pkmaps(void)
{
	int i;
	int need_flush = 0;

	flush_cache_kmaps();

	for(i = 0 ; i < LAST_PKMAP ; i++) {
		struct page *page;

		/*
		 * zero means we don't have anything to do,
		 * >1 means that it is still in use.Only
		 * a count of 1 means that it is free but
		 * needs to be unmapped
		 */
		if(pkmap_count[i] != 1)
			continue;
		pkmap_count[i] = 0;

		/* sanity check */
	//	BUG_ON(pte_none(pkmap_page_table[i]));

		/*
		 * Don't need an atomic fetch-and-clear op here;
		 * no-one has the page mapped,and cannot get at
		 * its virtual address(and hence PTE) without first
		 * getting the kmap_lock(which is held here).
		 * So no dangers,event with speculative execution.
		 */
//		page = pte_page(pkmap_page_table[i]);
//		pte_clear(&init_mm,(unsigned long)page_address(page),
//				&pkmap_page_table[i]);

		set_page_address(page,NULL);
		need_flush = 1;
	}
	if(need_flush)
		flush_tlb_kernel_range(PKMAP_ADDR(0),PKMAP_ADDR(LAST_PKMAP));
}

static inline unsigned long map_new_virtual(struct page *page)
{
	unsigned long vaddr;
	int count;

start:
	count = LAST_PKMAP;
	/* Find any empty entry */
	for(;;) {
		last_pkmap_nr = (last_pkmap_nr + 1) & LAST_PKMAP_MASK;
		if(!last_pkmap_nr) {
			flush_all_zero_pkmaps();
			count = LAST_PKMAP;
		}
		if(!pkmap_count[last_pkmap_nr])
			break;   /* Found a usable entry */
		if(--count)
			continue;
		
		/* Can't sleep !*/
	}
	vaddr = PKMAP_ADDR(last_pkmap_nr);
	set_pte_at(&init_mm,vaddr,
			&(pkmap_page_table[last_pkmap_nr]),mk_pte(page,kmap_prot));

	pkmap_count[last_pkmap_nr] = 1;
	set_page_address(page,(void *)vaddr);

	return vaddr;
}

/**
 * kmap_high - map a highmem page into memory
 * @page:&struct page to map
 *
 * Return the page's virtual memory address.
 *
 * We cannot call this from interrupts,as it may block.
 */
void *kmap_high(struct page *page)
{
	unsigned int vaddr;

	/* 
	 * For highmem pages,we can't trust "virtual" until
	 * after we have the lock.
	 */
	lock_kmap();
	vaddr = (unsigned int)(unsigned long)page_address(page);
	if(!vaddr)
		vaddr = map_new_virtual(page);
	pkmap_count[PKMAP_NR(vaddr)]++;
	BUG_ON(pkmap_count[PKMAP_NR(vaddr)] < 2);
	unlock_kmap();
	return (void *)(unsigned long)vaddr;
}

/**
 * kunmap_high - map a highmem page into memory
 * @page:&struct page to unmap
 *
 * If ARCH_NEED_KMAP_HIGH_GET is not defined then this may be called
 * only from user context.
 */
void kunmap_high(struct page *page)
{
	unsigned long vaddr;
	unsigned long nr;
	unsigned long flags;
	int need_wakeup;

	lock_kmap_any(flags);
	vaddr = (unsigned long)page_address(page);
	BUG_ON(!vaddr);
	nr = PKMAP_NR(vaddr);

	/*
	 * A count must never go down to zero
	 * without a TLB flush.
	 */
	need_wakeup = 0;
	switch(--pkmap_count[nr]) {
		case 0:
			BUG();
		case 1:
			/*
			 * Avoid an unnecessary wake_up() function call.
			 * The common case is pkmap_count[] == 1,but
			 * no waiters.
			 * The tasks queued in the wait-queue are guarded
			 * by both the lock in the wait-queue-head and by
			 * the kmap_lock.As the kmap_lock is held here,
			 * no need for the wait-queue-head's lock.Simply
			 * test if the queue is empty.
			 */
			; // need_wakeup = waitqueue_active(&pkmap_map_wait);
	}
	lock_kmap_any(flags);

	/* do wake-up,if needed,race-free outside of the spin lock */
	if(need_wakeup)
		//wake_up(&pkmap_map_wait);
		;
}

void *kmap(struct page *page)
{
	might_sleep();
	if(!PageHighMem(page))
		return page_address(page);
	return kmap_high(page);
}

void kunmap(struct page *page)
{
	BUG_ON(in_interrupt());
	if(!PageHighMem(page))
		return;
	kunmap_high(page);
}
