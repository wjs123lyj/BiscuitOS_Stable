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
	struct hlist_head *head;
	struct hlist_node *hnode;
	struct node *node;
	int *value;

	/* Get the test data */
	value = (int *)kmalloc(sizeof(int) * VALUE_NUM , GFP_KERNEL);
	for(i = 0 ; i < VALUE_NUM ; i++) 
		value[i] = i;
		
	/* Allocate memory to Hash Head */
	phash = (struct hlist_head *)kmalloc(
			sizeof(struct hlist_head) * (1 << HEAD_NUM),GFP_KERNEL);

	/* Initialize hash head */
	for(i = 0 ; i < (1 << HEAD_NUM) ; i++)
		INIT_HLIST_HEAD(phash + i);

	/* Add test data into hash list */
	for(i = 0 ; i < VALUE_NUM ; i++) {
		node = (struct node *)kmalloc(sizeof(struct node),GFP_KERNEL);
		if(!node) {
			mm_err("No memory\n");
			
			/* Never water memory */
			goto bad;
		}

		/* Set the node value */
		node->num = value[i];
		/* Get the hash head */
		head = &phash[hash_32(node->num,HEAD_NUM)];
		/* Initialize hash node */
		INIT_HLIST_NODE(&node->node);
		/* Add node into hash list */
		hlist_add_head(&node->node,head);
	}

	/* Trave all node */
	for(i = 0 ; i < (1 << HEAD_NUM) ; i++) {
		head = phash + i;
		mm_debug("Head %3d :",i);
		if(!hlist_empty(head)) 
			hlist_for_each_entry(node,hnode,head,node)
				mm_debug("%d->",node->num);
		mm_debug("NULL\n");
	}

	k = value[5];
	/* Search data from Hash list */
	head = &phash[hash_32(k,HEAD_NUM)];
	hlist_for_each_entry(node,hnode,head,node) 
		if(node->num == k) 
			mm_debug("Find data %d\n",k);

    /* Free all hlist node */
	for(i = 0 ; i < (1 << HEAD_NUM) ; i++) {
		head = phash + i;
		while(!hlist_empty(head)) {
			node = hlist_entry(head->first,struct node,node);
			hlist_del(&node->node);
			kfree(node);
		}
	}

	/* Free check */
	mm_debug("After free operation\n");
	for(i = 0 ; i < (1 << HEAD_NUM) ; i++) {
		head = phash + i;
		if(!hlist_empty(head))
			hlist_for_each_entry(node,hnode,head,node)
				mm_debug("Remain node %d\n",node->num);
	}
	/* Final free */
	kfree(phash);
	kfree(value);

	return;
bad:
	for(i-- ; i >= 0 ; i--) {
		head = &phash[hash_32(value[i],HEAD_NUM)];
		hlist_for_each_entry(node,hnode,head,node) 
			kfree(node);
	}
	kfree(phash);
	kfree(value);
}
