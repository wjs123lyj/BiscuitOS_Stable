#include "../include/linux/list.h"
#include "../include/linux/debug.h"

inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->prev = list;
	list->next = list;
}

/*
 * Underlying list_add
 */
inline void __list_add(struct list_head *new,
		struct list_head *prev,
		struct list_head *next)
{
	prev->next = new;
	new->prev = prev;
	new->next = next;
	next->prev = new;
}
/*
 * Add 
 */
inline void list_add(struct list_head *new,struct list_head *head)
{
	__list_add(new,head,head->next);
}
/*
 * Remove underlying
 */
inline void __list_del(struct list_head *prev,struct list_head *next)
{
	prev->next = next;
	next->prev = prev;
}
/*
 * Delete member
 */
inline void list_del(struct list_head *list)
{
	__list_del(list->prev,list->next);
}




