/*
 * This file is used to debug rbtree.
 * We give a simple demo code for unsing rbtree API.
 * The code from http://blog.csdn.net/npy_lp/article/details/7420689.
 *
 * Thanks tanglinux.
 * Create By Buddy.D.Zhang
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
 * Insert the node into RBtree.
 */
int insert(struct node *data,struct rb_root *root)
{
	struct rb_node **link = &(root->rb_node);
	struct rb_node *parent = NULL;

	while(*link) {
		struct node *node = container_of(*link,struct node,node);

		parent = *link;
		if(data->num < node->num)
			link = &((*link)->rb_left);
		else if(data->num > node->num)
			link = &((*link)->rb_right);
		else
			return -1;
	}
	rb_link_node(&data->node,parent,link);
	rb_insert_color(&data->node,root);
	return 0;
}

/*
 * Search the node from RBtree.
 */
struct node *search(int num,struct rb_root *root)
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
 * Delete the node from RBtree.
 */
void delete(int num,struct rb_root *root)
{
	struct node *node = search(num,root);
	
	if(!node) {
		mm_err("%d doesn't exist\n",num);
		return;
	}
	rb_erase(&node->node,root);
	kfree(node);
}

/*
 * Print all node of RBtree.
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
	struct node *node;
	int num,i,ret;
	int value[10] = { 2 , 4, 123 , 43 , 56 , 78 , 32 , 17 , 51 , 1 };

	num = 10;

	for(i = 0 ; i < num ; i++) {
		node = (struct node *)kmalloc(sizeof(struct node),GFP_KERNEL);
		if(!node) {
			mm_err("No Memory\n");

			/* Don't waste any memory */
			for(i-- ; i >= 0 ; i--) 
				delete(value[i],&root);
			return;
		}

		node->num = value[i];

		/* Insert node into rbtree */
		ret = insert(node,&root);
		if(ret < 0) {
			mm_debug("%2d has existed\n",node->num);
			kfree(node);
		}
	}

	mm_debug("First Check\n");
	print_rbtree(&root);
}
