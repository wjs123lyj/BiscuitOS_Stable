#ifndef _BITOPS_H_
#define _BITOPS_H_
#include "kernel.h"

#define BITS_PER_LONG   32

#define BIT(nr)              (unsigned int)(1UL << (nr))
#define BIT_MASK(nr)         (unsigned int)(1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)         ((nr) / BITS_PER_LONG)
#define BITS_PER_BYTE        8
#define BITS_TO_LONGS(nr)    DIV_ROUND_UP(nr,BITS_PER_LONG)

extern int find_next_bit(unsigned int *addr,unsigned long size,
		unsigned long offset);

#define find_first_bit(addr,size) find_next_bit((addr),(size),0)

#define for_each_set_bit(bit,addr,size)     \
	for((bit) = find_first_bit((addr),(size));    \
			(bit) < (size);   \
			(bit) = find_next_bit((addr),(size),(bit) + 1))

#define DECLARE_BITMAP(name,bits) \
	unsigned int name[BITS_TO_LONGS(bits)]
#endif
