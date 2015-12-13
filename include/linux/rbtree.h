#ifndef _RBTREE_H_
#define _RBTREE_H_
#include "kernel.h"

struct rb_node {
	unsigned long rb_parent_color;
#define RB_RED     0
#define RB_BLACK   1
	struct rb_node *rb_right;
	struct rb_node *rb_left;
}__attribute__((aligned(sizeof(long))));
/* The alignment might seem pointless,but allegedly CRIS need it */

struct rb_root {
	struct rb_node *rb_node;
};

#define rb_parent(r)  ((struct rb_node *)((r)->rb_parent_color & ~3))
#define rb_color(r)   ((r)->rb_parent_color & 1)
#define rb_is_red(r)  (!rb_color(r))
#define rb_is_block(r)  rb_color(r)
#define rb_set_red(r)  do { (r)->rb_parent_color &= ~1;} while(0)
#define rb_set_block(r) do{ (r)->rb_parent_color |= 1;} while(0)

static inline void rb_set_parent(struct rb_node *rb,struct rb_node *p)
{
	rb->rb_parent_color = (rb->rb_parent_color & 3) | (unsigned long)p;
}
static inline void rb_set_color(struct rb_node *rb,int color)
{
	rb->rb_parent_color = (rb->rb_parent_color & ~1) | color;
}

#define RB_EMPTY_NODE(node) (rb_parent(node) == node)

#define RB_ROOT  (struct rb_root) {NULL,}
#define rb_entry(ptr,type,member) container_of(ptr,type,member)

#define RB_EMPTY_ROOT(root)  ((root)->rb_node == NULL)
#define RB_EMPTY_NODE(node)  (rb_parent(node) == node)
#define RB_CLEAR_NODE(node)  (rb_set_parent(node,node))

static inline void rb_link_node(struct rb_node *node,struct rb_node *parent,
		struct rb_node **rb_link)
{
	node->rb_parent_color = (unsigned long)parent;
	node->rb_left = node->rb_right = NULL;

	*rb_link = node;
}

extern struct rb_node *rb_next(struct rb_node *node);
extern struct rb_node *rb_prev(struct rb_node *node);
#endif
