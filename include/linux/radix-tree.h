#ifndef _RADIX_TREE_H_
#define _RADIX_TREE_H_

#include "types.h"
/*** Radix tree API start here ***/
#define RADIX_TREE_MAX_TAGS     3

/*
 * An indirect pointer(root->rnode pointing to a radix_tree_node,rather
 * than a data item) is signalled by the low bit set in the root->rnode
 * pointer.
 *
 * In this case root->height is > 0,but the indirect pointer tests are
 * needed for RCU lookups(because root->height is unreliable).The only
 * time callers need worry about this is when doing a lookup_slot under
 * RCU.
 *
 * Indirect pointer in fact is also used to tag the last pointer of a node
 * when it is shrunk,before we rcu free the node.See shrink code for
 * details.
 */
#define RADIX_TREE_INDIRECT_PTR   1

#define radix_tree_indirect_to_ptr(ptr)    \
	radix_tree_indirect_to_ptr((void *)(ptr))

static inline int radix_tree_is_indirect_ptr(void *ptr)
{
	return (int)((unsigned long)ptr & RADIX_TREE_INDIRECT_PTR);
}



/* root tags are shored in gfp_mask,shifted by __GFP_BITS_SHIFT */
struct radix_tree_root {
	unsigned int height;
	gfp_t gfp_mask;
	struct radix_tree_node *rnode;
};

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

#endif
