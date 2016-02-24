#ifndef _RMAP_H_
#define _RMAP_H_

#include "linux/spinlock.h"
#include "linux/list.h"

/*
 * The anon_vma heads a list of private "related" vmas,to scan if 
 * an anonymous page pointing to this anon_vma needs to be unmapped
 * the vmas on the list will related by forking,or by splitting.
 *
 * Since vmas come and go as they are split and merged(particularly
 * in mprotect),the mapping field of an anonymous page cannot point
 * directly to a vma:instead it points to an anon_vma,on whose list
 * the related vmas can be easily linkly linked or unlinked.
 *
 * After unlinking the last vma on the list,we must garbage collect
 * the anon_vma object itself:we're guarateed no page can be 
 * pointing to this anon_vma once its vma list is empty.
 */
struct anon_vma {
	struct anon_vma *root;  /* Root of this anon_vma tree */
	spinlock_t lock;  /* Seralize access to vma list */
	
	/*
	 * NOTE:the LSB of the head.next is set by
	 * mm_take_all_locks()_after_taking the above lock.So the 
	 * head must only be read/written after taking the above lock
	 * to be sure to see a valid next pointer.The LSB bit itself
	 * is serialized by a system wide lock only visible to 
	 * mm_take_all_locks()(mm_all_locks_mutex).
	 */
	struct list_head head;  /* Chain of private "related" vmas */
};

/*
 * The copy-on-write semantics of fork mean that an anon_vma
 * can become associated with multiple pricesses.Furthermore,
 * each child process will have its own anon_vma,where new
 * page for that process are instantiated.
 *
 * This structure allows us to find the anon_vmas associated
 * with a VMA,or the VMAs associated with an anon_vma.
 * The "same_vma" list contains the anon_vma_chains linking
 * all the anon_vmas associated with this VMA.
 * The "same_anon_vma" list contains the anon_vma_chains
 * which link all the VMAs associated with this anon_vma.
 */
struct anon_vma_chain {
//	struct vm_area_struct *vma;
	struct anon_vma *anon_vma;
	struct list_head same_vma; /* locked by mmap_sem & page_table_lock */
	struct list_head same_anon_vma;  /* locked by anon_vma->lock */
};

static inline void anonvma_external_refcount_init(struct anon_vma *anon_vma)
{
}

#endif
