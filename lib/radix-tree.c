#include "linux/kernel.h"
#include "linux/radix-tree.h"
#include "linux/gfp.h"
#include "linux/rcupdate.h"
#include "linux/debug.h"
#include "linux/slab.h"
#include "linux/notifier.h"
#include "linux/cpu.h"


#define RADIX_TREE_MAP_SHIFT     3

#define RADIX_TREE_MAP_SIZE    (1UL << RADIX_TREE_MAP_SHIFT)
#define RADIX_TREE_MAP_MASK    (RADIX_TREE_MAP_SIZE)

#define RADIX_TREE_TAG_LONGS      \
	((RADIX_TREE_MAP_SIZE + BITS_PER_LONG - 1) / BITS_PER_LONG)

/*
 * Radix tree node cache.
 */
static struct kmem_cache *radix_tree_node_cachep;

struct radix_tree_node {
	unsigned int height;    /* Height from the bottom */
	unsigned int count;
	struct rcu_head rcu_head;
	void __rcu *slots[RADIX_TREE_MAP_SIZE];
	unsigned long tags[RADIX_TREE_MAX_TAGS][RADIX_TREE_TAG_LONGS];
};

struct radix_tree_path {
	struct radix_tree_node *node;
	int offset;
};

#define RADIX_TREE_INDEX_BITS  ( 8 * sizeof(unsigned long))
#define RADIX_TREE_MAX_PATH  (DIV_ROUND_UP(RADIX_TREE_INDEX_BITS,   \
			RADIX_TREE_MAP_SHIFT))
/*
 * Per-cpu pool of preloaded nodes.
 */
struct radix_tree_preload {
	int nr;
	struct radix_tree_node *nodes[RADIX_TREE_MAX_PATH];
};
static DEFINE_PER_CPU(struct radix_tree_preload,radix_tree_preloads) = {0,};

/*
 * The height_to_maximum array needs to be one deeper than the maximum
 * path as height 0 holds only 1 entry.
 */
static unsigned long height_to_maxindex[RADIX_TREE_MAX_PATH + 1]; 

/****************************************************************
 *      Helper Function
 ***************************************************************/
static inline void *ptr_to_indirect(void *ptr)
{
	return (void *)((unsigned long)ptr | RADIX_TREE_INDIRECT_PTR);
}
static inline void *indirect_to_ptr(void *ptr)
{
	return (void *)((unsigned long)ptr & ~RADIX_TREE_INDIRECT_PTR);
}
static inline int root_tag_get(struct radix_tree_root *root,
		unsigned int tag)
{
	return (__force unsigned)root->gfp_mask & (1 << (tag + __GFP_BITS_SHIFT));
}
static inline void root_tag_clear(struct radix_tree_root *root,
		unsigned int tag)
{
	root->gfp_mask &= (__force gfp_t)~(1 << (tag + __GFP_BITS_SHIFT));
}
static inline void root_tag_clear_all(struct radix_tree_root *root)
{
	root->gfp_mask &= __GFP_BITS_MASK;
}

/*
 * Return the maximum key which can be store into a 
 * radix tree with heigh HIGHT.
 */
static inline unsigned long radix_tree_maxindex(unsigned int height)
{
	return height_to_maxindex[height];
}
static inline int tag_get(struct radix_tree_node *node,
		unsigned int tag,int offset)
{
	return test_bit(offset,node->tags[tag]);
}
static inline void tag_clear(struct radix_tree_node *node,unsigned int tag,
		int offset)
{
	clear_bit(offset,node->tags[tag]);
}
/*
 * Return q if any slot in the node has this tag set.
 * Otherwise returns 0.
 */
static inline int any_tag_set(struct radix_tree_node *node,unsigned int tag)
{
	int idx;
	
	for(idx = 0 ; idx < RADIX_TREE_TAG_LONGS ; idx++)
	{
		if(node->tags[tag][idx])
			return 1;
	}
	return 0;
}
/*
 * clear a tag on a radix tree node
 */
void *radix_tree_tag_clear(struct radix_tree_root *root,
		unsigned long index,unsigned int tag)
{
	/*
	 * The radix tree path needs to be one longer than the maximum path
	 * since the "list" is null terminated.
	 */
	struct radix_tree_path path[RADIX_TREE_MAX_PATH + 1],*pathp = path;
	struct radix_tree_node *slot = NULL;
	unsigned int height,shift;

	height = root->height;
	if(index > radix_tree_maxindex(height))
		goto out;

	shift = (height - 1) * RADIX_TREE_MAP_SHIFT;
	pathp->node = NULL;
	slot = indirect_to_ptr(root->rnode);

	while(height > 0)
	{
		int offset;

		if(slot == NULL)
			goto out;

		offset = (index >> shift) & RADIX_TREE_MAP_MASK;
		pathp[1].offset = offset;
		pathp[1].node   = slot;
		slot = slot->slots[offset];
		pathp++;
		shift -= RADIX_TREE_MAP_SHIFT;
		height--;
	}

	if(slot == NULL)
		goto out;

	while(pathp->node)
	{
		if(!tag_get(pathp->node,tag,pathp->offset))
			goto out;
		tag_clear(pathp->node,tag,pathp->offset);
		if(any_tag_set(pathp->node,tag))
			goto out;
		pathp--;
	}

	/* Clear the root's tag bit */
	if(root_tag_get(root,tag))
		root_tag_clear(root,tag);

out:
	return slot;
}
static void radix_tree_node_rcu_free(struct rcu_head *head)
{
	struct radix_tree_node *node = 
		container_of(head,struct radix_tree_node,rcu_head);
	int i;

	/*
	 * must only free zeroed nodes into the slab.radix_tree_shrink
	 * can leave us with a non-NULL entry in the first slot,so clear
	 * that here to make sure.
	 */
	for(i = 0 ; i < RADIX_TREE_MAX_TAGS ; i++)
		tag_clear(node,i,0);

	node->slots[0] = NULL;
	node->count = 0;

	kmem_cache_free(radix_tree_node_cachep,node);
}
/***********************************************************
 *      Core Function
 **********************************************************/
static inline void radix_tree_node_free(struct radix_tree_node *node)
{
	call_rcu(&node->rcu_head,radix_tree_node_rcu_free);
}
static inline void radix_tree_shrink(struct radix_tree_root *root);
/*
 * delete an item from a radix tree.
 */
void *radix_tree_delete(struct radix_tree_root *root,unsigned long index)
{
	/*
	 * Thr radix tree path needs to be one longer than the maximum path
	 * since the "list" is null terminated.
	 */
	struct radix_tree_path path[RADIX_TREE_MAX_PATH + 1],*pathp = path;
	struct radix_tree_node *slot = NULL;
	struct radix_tree_node *to_free;
	unsigned int height,shift;
	int tag;
	int offset;

	height = root->height;
	if(index > radix_tree_maxindex(height))
		goto out;

	slot = root->rnode;
	if(height == 0)
	{
		root_tag_clear_all(root);
		root->rnode = NULL;
		goto out;
	}
	slot = indirect_to_ptr(slot);

	shift = (height - 1) * RADIX_TREE_MAP_SHIFT;
	pathp->node = NULL;

	do {
		if(slot == NULL)
			goto out;

		pathp++;
		offset = (index >> shift) & RADIX_TREE_MAP_MASK;
		pathp->offset = offset;
		pathp->node = slot;
		slot = slot->slots[offset];
		shift -= RADIX_TREE_MAP_SHIFT;
		height--;
	} while(height > 0);

	if(slot == NULL)
		goto out;

	/*
	 * Clear all tags associated with the just - deleted item.
	 */
	for(tag = 0 ; tag < RADIX_TREE_MAX_TAGS ; tag++)
	{
		if(tag_get(pathp->node,tag,pathp->offset))
			radix_tree_tag_clear(root,index,tag);
	}

	to_free = NULL;
	/* Now free the nodes we do not need anymore */
	while(pathp->node)
	{
		pathp->node->slots[pathp->offset] = NULL;
		pathp->node->count--;
		/*
		 * Queue the node for deferred freeing after the 
		 * last reference to it disappears.
		 */
		if(to_free)
			radix_tree_node_free(to_free);

		if(pathp->node->count)
		{
			if(pathp->node == indirect_to_ptr(root->rnode))
				radix_tree_shrink(root);
			goto out;
		}
		/*
		 * Node with zero slots in use so free it.
		 */
		to_free = pathp->node;
		pathp--;
	}
	root_tag_clear_all(root);
	root->height = 0;
	root->rnode = NULL;
	if(to_free)
		radix_tree_node_free(to_free);

out:
	return slot;
}
/*
 * Shrink height of a radix tree to minimal.
 */
static inline void radix_tree_shrink(struct radix_tree_root *root)
{
	/* try to shrink tree height */
	while(root->height > 0)
	{
		struct radix_tree_node *to_free = root->rnode;
		void *newptr;

		BUG_ON(!radix_tree_is_indirect_ptr(to_free));
		to_free = indirect_to_ptr(to_free);

		/*
		 * The candidate node has more than one child,or its child
		 * is not at the leftmost slot,we cannot shrink.
		 */
		if(to_free->count != 1)
			break;
		if(!to_free->slots[0])
			break;

		/*
		 * We don't need rcu_assign_pointer(),since we are simply
		 * moving the node from one part of the tree to another.If it
		 * was safe to dereference the old pointer to it 
		 * it will safe to dereference the new one as far as 
		 * dependent read barriers go.
		 */
		newptr = to_free->slots[0];
		if(root->height > 1)
			newptr = ptr_to_indirect(newptr);
		root->rnode = newptr;
		root->height--;

		/*
		 * We have a dilemma here.The node's slot[0] must not be
		 * NULLed in case there are concurrent loolups expecting to 
		 * find the item.However if this was a bottom-level node,
		 * then if may be subject to the slot pointer being visible
		 * to callers dereferencing it.If item corresponding to 
		 * slot[0] is subsequently deleted,these callers would expect
		 * theri slot to become empty sooner or later.
		 *
		 * For example,lockless pagecache will look up a slot,deref
		 * the page pointer,and if the page is 0 refcount if means it 
		 * was concurrently deleted from pagecache so try the deref
		 * again.Fortunately there is already a requirement for logic
		 * to retry the entry slot lookup --  the indirect pointer
		 * problem/So tag the slot as indirect to force caller to retry.
		 */
		if(root->height == 0)
			*((unsigned long *)&to_free->slots[0])  |=
				RADIX_TREE_INDIRECT_PTR;

		radix_tree_node_free(to_free);
	}
}

static __init unsigned long __maxindex(unsigned int height)
{
	unsigned int width = height * RADIX_TREE_MAP_SHIFT;
	int shift = RADIX_TREE_INDEX_BITS - width;

	/**
	 * Need more debug...Should use 0U not 0UL?
	 */
	if(shift < 0)
		return ~0UL;
	if(shift >= BITS_PER_LONG)
		return 0UL;
	return ~0UL >> shift;
}

static __init void radix_tree_init_maxindex(void)
{
	unsigned int i;

	for(i = 0 ; i < ARRAY_SIZE(height_to_maxindex) ; i++)
		height_to_maxindex[i] = __maxindex(i);
}

static void radix_tree_node_ctor(void *node)
{
	memset(node,0,sizeof(struct radix_tree_node));
}

static int radix_tree_callback(struct notifier_block *nfb,
		unsigned long action,
		void *hcpu)
{
	int cpu = (long)hcpu;
	struct radix_tree_preload *rtp;

	/* Free per-cpu pool of perloaded nodes */
	if(action == CPU_DEAD || action == CPU_DEAD_FROZEN) {
		rtp = &per_cpu(radix_tree_preloads,cpu);
		while(rtp->nr) {
			kmem_cache_free(radix_tree_node_cachep,
					rtp->nodes[rtp->nr - 1]);
			rtp->nodes[rtp->nr - 1] = NULL;
			rtp->nr--;
		}
	}
	return NOTIFY_OK;
}

void __init radix_tree_init(void)
{
	radix_tree_node_cachep = (struct kmem_cache *)
		(unsigned long)(kmem_cache_create("radix_tree_node",
			sizeof(struct radix_tree_node),0,
			SLAB_PANIC | SLAB_RECLAIM_ACCOUNT,
			radix_tree_node_ctor));
	radix_tree_init_maxindex();
	hotcpu_notifier(radix_tree_callback,0);
}
