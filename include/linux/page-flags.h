#ifndef _PAGE_FLAGS_H_
#define _PAGE_FLAGS_H_

#include "bounds.h"
#include "mm_types.h"
/*
 * Various page->flags bits:
 * PG_reserved is set for special pages,which can never be swapped out.Some
 * of them might not even exist(eg empty_bad_page)...
 *
 * The PG_private bitflag is set on pagecache pages if they contain filesystem
 * specific data(which is normally at page->private).It can be used by 
 * private allocations for its own usage.
 *
 * During initation of disk I/O,PG_locked is set.This bit is set before I/O
 * and cleared when writeback _start_ or when read _completes_. PG_writeback
 * is set before writeback starts and cleared when it finishes.
 *
 * PG_locked also pins a page in pagecache,and blocks truncation of the file
 * while it is held.
 *
 * page_waitqueue(page) is a wait queue of all tasks waiting for the page
 * to become unlocked.
 *
 * PG_uptodate tells whether the page's contents is valid.When a read
 * completes,the page becomes uptodate,unless a disk I/O error happened.
 *
 * PG_referenced,PG_reclaim are used for page reclaim for anonymous and 
 * file-backed pagecache.
 *
 * PG_error is set to indicate that an I/O error occurred on this page.
 *
 * PG_arch_1 is an architecture specific page state bit.The generic code
 * guarantees that this bit is cleared for a page when it first is entered
 * into the page cache.
 *
 * PG_highmem pages are not permanently mapped into the kernel virtual address
 * space,they need to be kmapped separately for doing IO on the pages.The
 * struct page are always mapped into kernel address space...
 *
 * PG_hwpoison indicates that a page got corrupted in hardware and contains
 * data with incorrect ECC bits that triggered a machine check.Accessing is 
 * not safe since it may cause another machine check.Don't touch.
 */
enum pageflags {
	PG_locked,         /* Page is locked.Don't touch. */
	PG_error,
	PG_referenced,
	PG_uptodate,
	PG_dirty,
	PG_lru,
	PG_active,
	PG_slab,
	PG_owner_priv_1,   /* Owner use.If pagecache,fs may use */
	PG_arch_1,
	PG_reserved,
	PG_private,        /* If pagecache,has fs-private data */
	PG_private_2,      /* If pagecache,has fs aux data */
	PG_writeback,      /* Page is under writeback */
#ifdef CONFIG_PAGEFLAGS_EXTENDED
	PG_head,           /* A head page */
	PG_tail,           /* A tail page */
#else
	PG_compound,       /* A compound page */
#endif
	PG_swapcache,      /* Swap page:swp_entry_t in private */
	PG_mappedtodisk,   /* Has blocks allocated on-disk */
	PG_reclaim,        /* To be reclaimed asap */
	PG_swapbacked,     /* Page is backed by RAM /swap */
	PG_unevictable,    /* Page is "unevictable " */
#ifdef CONFIG_MMU
	PG_mlocked,        /* Page is vma mlocked */
#endif
#ifdef CONFIG_ARCH_USES_PG_UNCACHED
	PG_uncached,       /* Page has been mapped as uncached */
#endif
#ifdef CONFIG_MEMORY_FAILURE
	PG_hwpoison,       /* Hardware poisoned page */   
#endif
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	PG_comound_lock,
#endif
	__NR_PAGEFLAGS,
};

/*
 * Macro to crate function definitions for page flags.
 */
#define TESTPAGEFLAG(uname,lname)        \
static inline int Page##uname(struct page *page)    \
{   return test_bit(PG_##lname,&page->flags);} 


#define SETPAGEFLAG(uname,lname)         \
static inline void SetPage##uname(struct page *page)       \
{	set_bit(PG_##lname,&page->flags);}


#define CLEARPAGEFLAG(uname,lname)              \
static inline void ClearPage##uname(struct page *page)    \
{   clear_bit(PG_##lname,&page->flags);}


#define __SETPAGEFLAG(uname,lname)            \
static inline void __SetPage##uname(struct page *page)         \
{	set_bit(PG_##lname,&page->flags);}

#define __CLEARPAGEFLAG(uname,lname)    \
static inline void __ClearPage##uname(struct page *page)     \
{	clear_bit(PG_##lname,&page->flags);}

#define TESTSETFLAG(uname,lname)                 \
static inline int TestSetPage##uname(struct page *page)     \
{	return test_and_set_bit(PG_##lname,&page->flags);}


#define TESTCLEARFLAG(uname,lname)              \
static inline int TestClearPage##uname(struct page *page)  \
{   return test_and_clear_bit(PG_##lname,&page->flags);}


#define __TESTCLEARFLAG(uname,lname)          \
static inline int __TestClearPage##uname(struct page *page)  \
{   return test_and_clear_bit(PG_##lname,&page->flags);}


#define PAGEFLAG(uname,lname) TESTPAGEFLAG(uname,lname)      \
	SETPAGEFLAG(uname,lname)  CLEARPAGEFLAG(uname,lname)

#define __PAGEFLAG(uname,lname) TESTPAGEFLAG(uname,lname)     \
	__SETPAGEFLAG(uname,lname)   __CLEARPAGEFLAG(uname,lname)

#define PAGEFLAG_FALSE(uname)      \
static inline int Page##uname(struct page *page)     \
{   return 0;}


#define TESTSCFLAG(uname,lname)   \
	TESTSETFLAG(uname,lname)  TESTCLEARFLAG(uname,lname)

struct page;     /* forward declaration */

PAGEFLAG(Reserved,reserved) __CLEARPAGEFLAG(Reserved,reserved)

__PAGEFLAG(SlubFrozen,slub_frozen)

#ifdef CONFIG_MMU
PAGEFLAG(Mlocked,mlocked) __CLEARPAGEFLAG(Mlocked,mlocked)    
	TESTSCFLAG(Mlocked,mlocked) __TESTCLEARFLAG(Mlocked,mlocked)
#else
PAGEFLAG_FALSE(Mlocked,mlocked) __CLEARPAGEFLAG(Mlocked,mlocked)
	TESTCLEARFLAG_FALSE(Mlocked) __TESTCLEARFLAGS_FALSE(Mlocked)
#endif

#ifdef CONFIG_HIGHMEM
/*
 * Must use a macro here due to header dependency issues.page_zone() is not
 * available at this point.
 */
#define PageHighMem(__p) is_highmem(page_zone(__p))

#else
PAGEFLAG_FALSE(HighMem)
#endif

#ifdef CONFIG_MMU
#define __PG_MLOCKED         (1 << PG_mlocked)
#else
#define __PG_MLOCKED         0
#endif

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
#define __PG_COMPOUND_LOCK   (1 << PG_compound_lock)
#else
#define __PG_COMPOUND_LOCK   0
#endif

/*
 * Flags checked when a page is freed.Pages being freed should not have
 * these flags set.It they are,there is a problem.
 */
#define PAGE_FLAGS_CHECK_AT_FREE      \
	(1 << PG_lru          | 1 << PG_locked       | \
	 1 << PG_private      | 1 << PG_private_2    | \
	 1 << PG_writeback    | 1 << PG_reserved     | \
	 1 << PG_slab         | 1 << PG_swapcache    | \
	 1 << PG_active       | 1 << PG_unevictable  | \
	 __PG_MLOCKED         | __PG_HWPOISON        | \
	 __PG_COMPOUND_LOCK)


/*
 * Flags checked when a page is prepped for return by the page allocator.
 * Pages being prepped should not have any flags set.It they are set,
 * there has been a kernel bug or struct page corruption.
 */
#define PAGE_FLAGS_CHECK_AT_PREP     ((1 << NR_PAGEFLAGS) - 1)

/*
 * System with lots of page flags available.This allows separate
 * flags for PageHead() and PageTail() checks of compound pages so 
 * that bit tests can be used in performance sensitive paths.PageCompound
 * is generally not used in hot code paths.
 */
#ifdef CONFIG_PAGEFLAGS_EXTENDED
/*
 * System with lost of page flags available.This allows separate
 * flags for PageHead() and PageTail() check of compound pages so that bit
 * test can be used in performance sensitive paths.PageCompound is
 * generally not used in hot code paths.
 */
__PAGEFLAG(Head,head) CLEARPAGEFLAG(Head,head)
__PAGEFLAG(Tail,tail)

static inline int PageCompound(struct page *page)
{
	return page->flags & ((1L << PG_head) | (1L << PG_tail));
}

#else
/*
 * Reduce page flag use as possible by overlapping
 * compound page flags with the flags used for page cache pages.
 * Possible because PageCompound is always set for compound pages and
 * not for pages on the LRU and/or pagecache.
 */
TESTPAGEFLAG(Compound,compound)
__PAGEFLAG(Head,compound)
/*
 * PG_reclaim is used in combination with PG_compound to mark the 
 * head and tail of a compound page.This saves one page flag
 * but makes it impossible to use compound pages for the page cache.
 * The PG_reclaim bit would have to be used for reclaim or readahead
 * if compound pages enter the page cache.
 *
 * PG_compound & PG_reclaim     => Tail page
 * PG_compound & ~PG_recaim     => Head page
 */
#define PG_head_tail_mask ((1L << PG_compound) | (1L << PG_reclaim))

static inline int PageTail(struct page *page)
{
	return ((page->flags & PG_head_tail_mask) == PG_head_tail_mask);
}

static inline void __ClearPageTail(struct page *page)
{
	page->flags &= ~PG_head_tail_mask;
}
#endif

#ifdef CONFIG_MEMORY_FAILURE
PAGEFLAG(HWPoison,hwpoison)
TESTSCFLAG(HWPoison,hwpoison)
#define __PG_HWPOISON (1UL << PG_hwpoison)
#else
PAGEFLAG_FALSE(HWPoison)
#define __PG_HWPOISON 0
#endif

#endif

