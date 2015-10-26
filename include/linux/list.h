#ifndef _LIST_H_
#define _LIST_H_
#include "kernel.h"

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

#define list_entry(ptr,type,member) container_of(ptr,type,member)

#define list_for_each_entry(pos,head,member)     \
		for(pos = list_entry((head)->next,typeof(*pos),member);   \
			&pos->member != (head) ;     \
			pos = list_entry((pos)->member.next,typeof(*pos),member))

#define list_for_each(pos,head) \
		for(pos = (head)->next;   \
				pos != (head) ; \
				pos = pos->next)

#endif
