#ifndef _GETORDER_H_
#define _GETORDER_H_

#include "../linux/page.h"

/* Pure 2^n version of get_order */
static inline int get_order(unsigned long size)
{
	int order;

	size = (size - 1) >> (PAGE_SHIFT - 1);
	order = -1;
	do {
		size >>= 1;
		order++;
	} while(size);
	
	return order;
}

#endif
