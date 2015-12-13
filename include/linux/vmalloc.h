#ifndef _VMALLOC_H_
#define _VMALLOC_H_
#include "types.h"
#include "rbtree.h"


struct vm_struct {
	struct vm_struct *next;
	void *addr;
	unsigned long size;
	unsigned long flags;
	struct page **pages;
	unsigned int nr_pages;
	phys_addr_t  phys_addr;
	void *caller;
};

struct vmap_area {
	unsigned long va_start;
	unsigned long va_end;
	unsigned long flags;
	struct rb_node rb_node;   /* Address sorted rbtree */
	struct list_head list;    /* address sorted list */
	struct list_head purge_list; /* lazy purge list */
	void *private;
	struct rcu_head rcu_head;
};

/*
 * Maximum alignment for ioremap() regions.
 * Can be overriden by arch-specific value.
 */
#define IOREMAP_MAX_ORDER  (7 + PAGE_SHIFT)   /* 128 pages */
/*
 * Bits in flags of vmalloc's vm_struct below.
 */
#define VM_IOREMAP   0x00000001  /* ioremap() and friends */   
#define VM_ALLOC     0x00000002  /* vmalloc() */
#define VM_MAP       0x00000004  /* vmap()ed pages */
#define VM_USERMAP   0x00000008  /* suitable for remap_vmalloc_range */
#define VM_VPAGES    0x00000010  /* buffer for pages was vmalloc'ed */













#endif
