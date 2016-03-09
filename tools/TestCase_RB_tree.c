/*
 * This file is used to debug rbtree,we give a simple demo code for rbtree.
 * The code from http://blog.csdn.net/npy_lp/article/details/7420689.
 *
 * Thanks tanglinux.
 */
#include "linux/kernel.h"
#include "linux/rbtree.h"
#include "linux/debug.h"
#include "linux/mm.h"
#include "linux/gfp.h"
#include "linux/slab.h"

struct node {
	struct rb_node node;
	int num;
};

/*
 * Insert a member info rbtree.
 */
int insert(struct rb_root *root,struct node *data)
{
	struct rb_node **node = &(root->rb_node);
	struct rb_node *parent = NULL;

	while(*node) {
		struct node *this_node = container_of(*node,struct node,node);

		parent = *node;
		if(data->num < this_node->num)
			node = &((*node)->rb_left);
		else if(data->num > this_node->num)
			node = &((*node)->rb_right);
		else
			return -1;
	}

	/* Add new node and rebalance tree */
	rb_link_node(&data->node,parent,node);
	rb_insert_color(&data->node,root);
}

/*
 * Search a node from RBtree.
 */
struct node *search(struct rb_root *root,int num)
{
	struct rb_node *node = root->rb_node;
	
	while(node) {
		struct node *data = container_of(node,struct node,node);
		
		if(num < data->num)
			node = node->rb_left;
		else if(num > data->num)
			node = node->rb_right;
		else
			return data;
	}
	
	return NULL;
}

/*
 * Delete a node from rbtree.
 */
void delete(struct rb_root *root,int num)
{
	struct node *data = search(root,num);
	
	if(!data) {
		mm_err("Not found %d\n",num);
		return;
	}

	rb_erase(&data->node,root);
	kfree(data);
}

/*
 * Print all node of rbtree.
 */
void print_rbtree(struct rb_root *root)
{
	struct rb_node *node;

	for(node = rb_first(root) ; node ; node = rb_next(node))
		mm_debug("%2d ",rb_entry(node,struct node,node)->num);

	mm_debug("\n");
}

/*
 * TestCase_RB_user 
 */
void TestCase_RB_user(void)
{
	struct rb_root root = RB_ROOT;
	int i,ret,num;
	struct node *node;
	int value[10] = {2,4,67,4,8,9,11,35,1,9};

	num = 10;
	for(i = 0 ; i < num ; i++) {
		node = (struct node *)kmalloc(sizeof(struct node),GFP_KERNEL);
		node->num = value[i];

		ret = insert(&root,node);
		if(ret < 0) {
			mm_err("The %d already exists.\n",node->num);
			kfree(node);
		}
	}

	mm_debug("The first test\n");
	print_rbtree(&root);

	delete(&root,4);

	mm_debug("The second test\n");
	print_rbtree(&root);
}
