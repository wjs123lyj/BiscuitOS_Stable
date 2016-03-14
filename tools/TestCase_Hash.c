/*
 * This demo code give a way how to use HASH Algorithm of Linux in your code.
 *
 * Create by Buddy.D.Zhang
 */
#include "linux/kernel.h"
#include "linux/debug.h"
#include "linux/types.h"
#include "linux/slab.h"

#define HEAD_NUM    3
#define VALUE_NUM   100

struct node {
	struct hlist_node node;
	int num;
};

struct hlist_head *phash = NULL;
/*
 * TestCase_Hash
 */
void TestCase_Hash(void)
{
	int i,k;
	struct hlist_node *hnode;
	struct hlist_head *head;
	struct node *node;
	int *value;

	/* Prepare test data */
	value = (int *)kmalloc(sizeof(int) * VALUE_NUM,GFP_KERNEL);
	for(i = 0 ; i < VALUE_NUM ; i++)
		value[i] = i;

	/* Prepare hash head array */
	phash = (struct hlist_head *)kmalloc(sizeof(struct hlist_head) *
					(1 << HEAD_NUM) , GFP_KERNEL);
	/* Initialize hash head */
	for(i = 0 ; i < (1 << HEAD_NUM) ; i++)
		INIT_HLIST_HEAD(&phash[i]);

	/* Create Test node */
	for(i = 0 ; i < VALUE_NUM ; i++) {
		node = (struct node *)kmalloc(sizeof(struct node),GFP_KERNEL);
		if(!node) {
			mm_err("No memory\n");

			/* Never water memory */
			goto bad_memory;
		}

		/* Prepare test data */
		node->num = value[i];
		/* Initialize the hash node */
		INIT_HLIST_NODE(&node->node);
		/* Get the hash head for node */
		head = &phash[hash_32(node->num,HEAD_NUM)];
		/* Add node into hash list */
		hlist_add_head(&node->node,head);
	}

	/* Trave all hash list */
	for(i = 0 ; i < (1 << HEAD_NUM) ; i++) {
		mm_debug("HEAD %d:",i);
		if(!hlist_empty(&phash[i]))
			hlist_for_each_entry(node,hnode,&phash[i],node)
				mm_debug("%d->",node->num);
		mm_debug("NULL\n");
	}

	/* Search data in hash list */
	k = value[34];
	head = &phash[hash_32(k,HEAD_NUM)];
	mm_debug("Search %d in head %d\n",k,hash_32(k,HEAD_NUM));
	if(!hlist_empty(head))
		hlist_for_each_entry(node,hnode,head,node)
			if(k == node->num)
				mm_debug("Find the data %d\n",k);

	/* Delete all node */
	for(i = 0 ; i < (1 << HEAD_NUM) ; i++)
		while(!hlist_empty(&phash[i])) {
			node = hlist_entry(phash[i].first,struct node,node);
			hlist_del(&node->node);
			kfree(node);
		}

	/* Final check */
	for(i = 0 ; i < (1 << HEAD_NUM) ; i++) {
		if(!hlist_empty(&phash[i])) {
			mm_debug("HEAD %d REMAIN:",i);
			hlist_for_each_entry(node,hnode,&phash[i],node)
				mm_debug("%d->",node->num);
			mm_debug("NULL\n");
		}
	}
	/* Free all and complete test */
	kfree(phash);
	kfree(value);

	return;
bad_memory:
	for(i = 0 ; i < (1 << HEAD_NUM) ; i++)
		while(!hlist_empty(&phash[i])) {
			node = hlist_entry(phash[i].first,struct node,node);
			hlist_del(phash[i].first);
			kfree(node);
		}
	kfree(phash);
	kfree(value);
}
