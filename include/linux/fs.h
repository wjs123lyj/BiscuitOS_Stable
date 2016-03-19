#ifndef _FS_H_
#define _FS_H_   1

#include "linux/radix-tree.h"
#include "linux/list.h"
#include "linux/spinlock_types.h"

/*
 * These ara the fs-independend mount-flags:up to 32 flags are supported.
 */
#define MS_RDONLY   1 /* Mount read-only */
#define MS_NOSUID   2 /* Ignore suid and sgid bits */
#define MS_NODEV    4 /* Disallow access to device specical files */
#define MS_NOEXEC   8 /* Disallow program execution */

#define MS_SILENT   32768

struct address_space {
	
	struct radix_tree_root page_tree; /* radix tree of all pages */
	spinlock_t tree_lock;    /* and lock protecting it */
	unsigned int i_mmap_writable; /* count VM_SHARED mappings */
	struct list_head i_mmap_nonlinear; /* list VM_NONLINEAR mappings */
	spinlock_t i_mmap_lock;  /* protect tree,count ,list */
	unsigned long nrpages;  /* number of total pages */
};

#endif
