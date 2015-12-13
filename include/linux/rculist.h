#ifndef _RCULIST_H_
#define _RCULIST_H_

#include "kernel.h"
#include "rcupdate.h"


#define __rcu_assign_pointer(p,v,space)   \
	({ \
	 if(!__builtin_constant_p(v) || \
		 ((v) != NULL))  \
	 smp_wmb(); \
	 (p) = (typeof(*v) __force space *)(v);  \
	 })
/*
 * Assign to RCU-protected pointer.
 */
#define rcu_assign_pointer(p,v)   \
	__rcu_assign_pointer((p),(v),__rcu)
/*
 * Return the ->next pointer of a list_head in an rcu safe
 * way,we must not access it directly.
 */
#define list_next_rcu(list)  (*((struct list_head __rcu **)(&(list)->next)))
/*
 * Insert a new entry between two known consecutive entries.
 */
static inline void __list_add_rcu(struct list_head *new,
		struct list_head *prev,struct list_head *next)
{
	new->next = next;
	new->prev = prev;
	rcu_assign_pointer(list_next_rcu(prev),new);
	next->prev = new;
}
/*
 * Add a new entry to rcu - protected list
 */
static inline void list_add_rcu(struct list_head *new,struct list_head *head)
{
	__list_add_rcu(new,head,head->next);
}
/*
 * Deletes entry from list without re-initialization.
 */
static inline void list_del_rcu(struct list_head *entry)
{
	__list_del(entry->prev,entry->next);
	entry->prev = LIST_POISON2;
}
/*
 * Get the struct for this entry
 */
#define list_entry_rcu(ptr,type,member)   \
	({typeof(*ptr) *__ptr = (typeof(*ptr) *)ptr;   \
	 container_of((typeof(ptr))rcu_dereference_raw(__ptr),type,member);  \
	 })
/*
 * iterate over rcu list of given type.
 */
#define list_for_each_entry_rcu(pos,head,member)     \
	for(pos = list_entry_rcu((head)->next,typeof(*pos),member);    \
			&pos->member != (head);     \
			pos = list_entry_rcu(pos->member.next,typeof(*pos),member))

#endif
