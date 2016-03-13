#ifndef _LIST_H_
#define _LIST_H_

#include "linux/poison.h"
#include "linux/prefetch.h"
#include "linux/types.h"
#include "linux/hash.h"


#define LIST_HEAD_INIT(name) { &(name),&(name)}

#define LIST_HEAD(name)  \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}
/*
 * Insert a new entry between two known consecutive entries.
 */
static inline void __list_add(struct list_head *new,
			struct list_head *prev,
			struct list_head *next)
{
	next->prev = new;
	new->next  = next;
	new->prev  = prev;
	prev->next = new;
}
/*
 * Add a new entry
 */
static inline void list_add(struct list_head *new,struct list_head *head)
{
	__list_add(new,head,head->next);
}
/*
 * Add a new entry before the specified head.
 */
static inline void list_add_tail(struct list_head *new,struct list_head *head)
{
	__list_add(new,head->prev,head);
}
/*
 * Delete a list entry by making the prev/next entries.
 */
static inline void __list_del(struct list_head *prev,struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}
/*
 * deletes entry from list.
 */
static inline void __list_del_entry(struct list_head *entry)
{
	__list_del(entry->prev,entry->next);
}
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev,entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}
/*
 * replace old entry by new one
 */
static inline void list_replace(struct list_head *old,
		struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}
static inline void list_replace_init(struct list_head *old,
		struct list_head *new)
{
	list_replace(old,new);
	INIT_LIST_HEAD(old);
}
/*
 * Deletes entry from list and reinitialize it.
 */
static inline void list_del_init(struct list_head *entry)
{
	__list_del_entry(entry);
	INIT_LIST_HEAD(entry);
}
/*
 * Delete from one list and add as another's head
 */
static inline void list_move(struct list_head *list,struct list_head *head)
{
	__list_del_entry(list);
	list_add(list,head);
}
/*
 * Delete from one list and add as another's tail
 */
static inline void list_move_tail(struct list_head *list,
		struct list_head *head)
{
	__list_del_entry(list);
	list_add_tail(list,head);
}
/*
 * test whether @list is the last entry in list @head
 */
static inline int list_is_last(const struct list_head *list,
		const struct list_head *head)
{
	return list->next == head;
}
/*
 * test wether a list is empty.
 */
static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

static inline void __list_splice(const struct list_head *list,
		struct list_head *prev,
		struct list_head *next)
{
	struct list_head *first = list->next;
	struct list_head *last  = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists,this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(const struct list_head *list,
		struct list_head *head)
{
	if(!list_empty(list))
		__list_splice(list,head,head->next);
}

/*
 * Get the struct for this entry.
 */
#define list_entry(ptr,type,member)  \
	container_of(ptr,type,member)
/*
 * Get the first element from a list
 */
#define list_fist_entry(ptr,type,member)  \
	list_entry((ptr)->next,type,member)
/*
 * iterate over a list
 */
#define list_for_each(pos,head) \
	for(pos = (head)->next ; prefetch(pos->next), pos != (head); \
			pos = pos->next)
/*
 * iterate over a list
 */
#define __list_for_each(pos,head)   \
	for(pos = (head)->next ; pos != (head) ; pos = pos->next)
/*
 * iterate over list of given type
 */
#define list_for_each_entry(pos,head,member)           \
	for(pos = list_entry((head)->next,typeof(*pos),member);       \
			prefetch(pos->member.next),&pos->member != (head);    \
			pos = list_entry(pos->member.next,typeof(*pos),member))
/*
 * iterate over list of given type safe against removal of list entry.
 */
#define list_for_each_entry_safe(pos,n,head,member)         \
	for(pos = list_entry((head)->next,typeof(*pos),member),   \
			n = list_entry(pos->member.next,typeof(*pos),member);  \
			&pos->member != (head);               \
			pos = n,n = list_entry(n->member.next,typeof(*n),member))

/**********************Hash List*****************************/

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * Ypu lose the ablility to access the tail in O(1).
 */

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) struct hlist_head name = { .first = NULL }
#define INIT_HLIST_HEAD(ptr)  ((ptr)->first = NULL)

static inline void INIT_HLIST_NODE(struct hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}

static inline void hlist_add_head(struct hlist_node *n,struct hlist_head *h)
{
	struct hlist_node *first = h->first;
	
	n->next = first;
	if(first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

static inline int hlist_empty(const struct hlist_head *h)
{
	return !h->first;
}

static inline void __hlist_del(struct hlist_node *n)
{
	struct hlist_node *next = n->next;
	struct hlist_node **pprev = n->pprev;
	*pprev = next;
	if(next)
		next->pprev = pprev;
}

static inline void hlist_del(struct hlist_node *n)
{
	__hlist_del(n);
	n->next = LIST_POISON1;
	n->pprev = LIST_POISON2;
}

#define hlist_entry(ptr,type,member) container_of(ptr,type,member)

/**
 * hlist_for_each_entry - iterate over list of given type
 * @tpos:  the type * to use as a loop cursor.
 * @pos:   the &struct hlist_node to use as a loop cursor.
 * @head:  the head for your list.
 * @member: the member of the hlist_node within the struct.
 */
#define hlist_for_each_entry(tpos,pos,head,member)    \
	for(pos = (head)->first;                             \
			pos && ({ prefetch(pos->next); 1 ;}) &&       \
			({ tpos = hlist_entry(pos,typeof(*tpos),member); 1 ;}); \
			pos = pos->next)


#endif
