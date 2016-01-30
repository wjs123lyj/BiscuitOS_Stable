#ifndef _BITMAP_H_
#define _BITMAP_H_ 1

#include "bitops.h"

#define BITMAP_LAST_WORD_MASK(nbits)       \
(                                          \
	((nbits) % BITS_PER_LONG)   ?          \
			(1UL << ((nbits) % BITS_PER_LONG)) - 1 : ~0UL       \
)

#define small_const_nbits(nbits)    \
	(__builtin_constant_p(nbits) && (nbits) <= BITS_PER_LONG)

static inline void bitmap_fill(unsigned long *dst,int nbits)
{
	size_t nlongs = BITS_TO_LONGS(nbits);
	if(!small_const_nbits(nbits)) {
		int len = (nlongs - 1) * sizeof(unsigned long);
		memset(dst,0xff,len);
	}
	dst[nlongs - 1] = BITMAP_LAST_WORD_MASK(nbits);
}

static inline void bitmap_copy(unsigned long *dst,const unsigned long *src,
		int nbits)
{
	if(small_const_nbits(nbits))
		*dst = *src;
	else {
		int len = BITS_TO_LONGS(nbits) * sizeof(unsigned long);
		memcpy(dst,src,len);
	}
}
#endif
