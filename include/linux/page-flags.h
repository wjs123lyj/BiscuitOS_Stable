#ifndef _PAGE_FLAGS_H_
#define _PAGE_FLAGS_H_

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
	PG_lry,
	PG_active,
	PG_slab,
	PG_owner_priv_1,   /* Owner use.If pagecache,fs may use */
	PG_arch_1,
	PG_reserved,
	PG_private,        /* If pagecache,has fs-private data */
	PG_private_2,      /* If pagecache,has fs aux data */
	PG_wirteback,      /* Page is under writeback */
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
 * Set the reserved_bit of page flags.
 */
static inline void SetPageReserved(struct page *page)
{
	set_bit(PG_reserved,&page->flags);
}
#if 0
/*
 * Macros to create function definitions for page flags.
 */
/* Test flags */
#define TESTPAGEFLAG(uname,lname)      \
static inline int Page##uname(struct page *page)     \
{	return test_bit(PG_##lname,&page->flags);}
/* Set flags */
#define SETPAGEFLAG(uname,lname)       \
static inline void SetPage##uname(struct page *page)  \
{	set_bit(PG_##lname,&page->flags);}
/* Clear flags */
#define CLEARPAGEFLAG(uname,lname)     \
static inline void ClearPage##uname(struct page *page) \
{	clear_bit(PG_##lname,&page->flags);}
/* __Set bit */
#define __SETPAGEFLAG(uname,lname)      \
static inline void __SetPage##uname(struct page *page)    \
{	set_bit(PG_##lname,&page->flags);}
/* __Clear bit */
#define __CLEARPAGEFLAG(uname,lname)       \
static inline void __ClearPage##uname(struct page *page)     \
{	clear_bit(PG_##lname,&page->flags);}
/* Test and clear flags */
#define TESTCLEARFLAGE(uname,lname)     \
static inline int TestClearPage##uname(struct page *page)    \
{	return test_and_clear_bit(PG_##lname,&page->flags);}
/* Test and set flags */
#define TESTSETFLAG(uname,lname)       \
static inline int TestSetPage##uname(struct page *page)   \
{	return test_and_set_bit(PG_##lname,&page->flags);}
/* __Test and clear flags */
#define __TESTCLEARFLAG(uname,lname)       \
static inline int __TestClearPage##uname(struct page *page)   \
{	return test_and_clear_bit(PG_##lname,&page->flags);}

/* Create page flags quickly */
#define PAGEFLAG(uname,lname)	TESTPAGEFLAG(uname,lname)     \
	SETPAGEFLAG(uname,lname)    CLEARPAGEFLAG(uname,lname)
/* Create page underlying flags quickly */
#define __PAGEFLAG(uname,lname) TESTPAGEFLAG(uname,lname)     \
	__SETPAGEFLAG(uname,lname)  __CLEARPAGEFLAG(uname,lname)
/* Test and clear flags */
#define TESTSCFLAG(uname,lname)      \
	TESTSETFLAG(uname,lname)  TESTCLEARFLAG(uname,lname)

struct page;  /* forward declaction */

/* Flags: Locked*/
TESTPAGEFLAG(Locked,locked) 
TESTSETFLAG(Locked,locked)
/* Flags: Error */
PAGEFLAG(Error,error)		
TESTCLEARFLAG(Error,error);
/* Flags: Referenced */
PAGEFLAG(Referenced,referenced)  TESTCLEARFLAG(Referenced,referenced)
/* Flags: Dirty */
PAGEFLAG(Dirty,dirty)       TESTCLEARFLAG(Dirty,dirty) 
/* Flags: LRU */
PAGEFLAG(LRU,lru)           __CLEARPAGEFLAG(LRU,lru)
/* Flags: Active */
PAGEFLAG(Active,active)     __CLEARPAGEFLAG(Active,active)  
	TESTCLEARFLAG(Active,active)
/* Flags: Slab */
__PAGEFLAG(Slab,slab)
/* Flags: Ckecked */
PAGEFLAG(Checked,checked)    /* Used by some filesystems */
/* Flags: Pinned */
PAGEFLAG(Pinned,pinned)      TESTSCFLAG(Pinned,pinned) /* Xen */
/* Flags: SavePinned */
PAGEFLAG(SavePinned,savepinned);
/* Flags: Reserved */
PAGEFLAG(Reserved,reserved)  __CLEARPAGEFLAG(Reserved,reserved)
/* Flags: SwapBacked */
PAGEFLAG(SwapBacked,swapbacked) __CLEARPAGEFLAGE(SwapBacked,swapbacked)
#endif
#endif
