#ifndef _RADIX_TREE_H_
#define _RADIX_TREE_H_

#include "types.h"
/*** Radix tree API start here ***/
#define RADIX_TREE_MAX_TAGS     3

/* root tags are shored in gfp_mask,shifted by __GFP_BITS_SHIFT */
struct radix_tree_root {
	unsigned int height;
	gfp_t gfp_mask;
	struct radix_tree_node *rnode;
};

/*
 * An indirect pointer is signalled by the low bit set
 * in the root->rnode pointer.
 */
#define RADIX_TREE_INDIRECT_PTR       1

#define RADIX_TREE_INIT(mask) {     \
	.height = 0,        \
    .gfp_mask = (mask),           \
    .rnode = NULL,          \
}


#define RADIX_TREE(name,mask)  \
	struct radix_tree_root name = RADIX_TREE_INIT(mask)

#define INIT_RADIX_TREE(root,mask)    \
	do {                              \
		(root)->height = 0;           \
		(root)->gfp_mask = (mask);    \
		(root)->rnode = NULL;         \
	} while(0)

static inline int radix_tree_is_indirect_ptr(void *ptr)
{
	return (int)((unsigned long)ptr & RADIX_TREE_INDIRECT_PTR);
}

#endif
