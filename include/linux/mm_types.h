#ifndef _MM_TYPES_H_
#define _MM_TYPES_H_

#include "page.h"
#include "types.h"
#include "list.h"
#include "fs.h"
#include "sched.h"
#include "spinlock_types.h"

/*
 * The page structure.
 */
struct page {
	unsigned long flags; /* Atomic flags,some possibly 
						  updated asynchronously */
	atomic_t _count;     /* Usage count */

	union {
		atomic_t _mapcount; /* Count of ptes mapped in mms,
							 * to show when page is mapped
							 * & limit reverse map searches.
							 */
		struct {            /* SLUB */
			u16 inuse;
			u16 objects;
		};
	};
	struct list_head lru;   /* Pageout list,eg,active_list
							 * protected by zone->lru_lock !
							 */
	union {
		struct {
			unsigned long private;
			                      /*
								   * Mapping-private opaque data.
								   * usually used for buffer_heads
								   * if PagePrivate set;used for
								   * swp_entry_t if PageSwapCache;
								   * indicates order in the buddy
								   * system if PG_buddy is set.
								   */
			struct address_space *mapping;
								  /*
								   * If low bit clear,points to inode 
								   * address_space,or NULL.
								   * If page mapped as anonymous
								   * memory,low bit is set,and
								   * it points to anon_vma object:
								   * set PAGE_MAPPING_ANON below,
								   */
		};
		struct kmem_cache *slab;
		struct page *first_page;  /* Compound tail pages */
	};
#ifdef CONFIG_WANT_PAGE_DEBUG_FLAGS
	unsigned long debug_flags;   /* Use atomic bitops on this */
#endif
	union {
		pgoff_t index;    /* Our offset within mapping */
		void *freelist;   /* SLUB:freelist req.slab lock */
	};
};
/*
 * mm_struct 
 */
struct mm_struct {
	pgd_t *pgd;
#ifdef CONFIG_MM_OWNER
	/*
	 * "owner" points to a task that is regarded as the canonical
	 * user/owner of this mm.All of the follow must be true true in
	 * order for it to be changed:
     *
	 * current == mm->owner
	 * current->mm != mm
	 * new_owner->mm == mm
	 * new_owner->alloc_lock is held
	 */
	struct task_struct *owner;
#endif
	spinlock_t page_table_lock;  /* Protects page tables and some counters */

	unsigned long total_vm,locked_vm,shared_vm,exec_vm;
	unsigned long stack_vm,reserved_vm,def_flags,nr_ptes;
	unsigned long start_code,end_code,start_data,end_data;
	unsigned long start_brk,brk,start_stack;
	unsigned long arg_start,arg_end,env_start,env_end;
};

#endif
