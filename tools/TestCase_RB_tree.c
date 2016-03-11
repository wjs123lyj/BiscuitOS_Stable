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
 * Insert a node into RBtree.
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
 * Search a node from RBtree.
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
 * Delete a node from RBtree.
 */
void delete(int num,struct rb_root *root)
{
	struct node *node = search(num,root);

	if(node) {
		rb_erase(&node->node,root);
		kfree(node);
	} else
		mm_err("%2d doesn't exits\n",num);
}

/*
 * Print all node 
 */
void print_all(struct rb_root *root)
{
	struct rb_node *node;

	for(node = rb_first(root) ; node ; node = rb_next(node))
		mm_debug("%2d  ",rb_entry(node,struct node,node)->num);

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
	int value[30] = { 2  , 84 , 43 , 11 , 7  , 54 , 21 , 1  , 2  , 10 ,
	                  34 , 5  , 6  , 45 , 76 , 0  , 12 , 25 , 44 , 11 ,
	                  99 , 65 , 38 , 91 , 35 , 16 ,93  , 74 , 33 , 67 };

	num = 30;

	for(i = 0 ; i < num ; i++) {
		node = (struct node *)kmalloc(sizeof(struct node),GFP_KERNEL);
		if(!node) {
			mm_err("No Memory\n");

			/* Never Waste memory */
			for(i-- ; i >= 0 ; i--)
				delete(value[i],&root);

			return;
		}

		node->num = value[i];

		/* Insert node into RBtree */
		ret = insert(node,&root);
		if(ret < 0) {
			mm_err("%2d has existed\n",node->num);
			kfree(node);
		}
	}

	mm_debug("First Check\n");
	print_all(&root);

	/* Delete a node */
	delete(value[4],&root);
	mm_debug("Second Check [%d]\n",value[4]);
	print_all(&root);

	/* Search a node */
	node = search(value[2],&root);
	mm_debug("Find %d\n",node->num);

    /* Get prev node */
	mm_debug("Prev num is %d\n",
			rb_entry(rb_prev(&node->node),struct node,node)->num);
	/* Get next node */
	mm_debug("Next num is %d\n",
			rb_entry(rb_next(&node->node),struct node,node)->num);
	/* The first node */
	mm_debug("Min num is %d\n",
			rb_entry(rb_first(&root),struct node,node)->num);
	/* The last node */
	mm_debug("Max num is %d\n",
			rb_entry(rb_last(&root),struct node,node)->num);

	/* Free All node */
	for(i = 0 ; i < num ; i++)
		delete(value[i],&root);

	mm_debug("Last Check\n");
	print_all(&root);
}
