#ifndef _LIST_H_
#define _LIST_H_
#include "kernel.h"
#include "poison.h"
#include "prefetch.h"

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

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
#endif
