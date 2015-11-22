#ifndef _ATOMIC_LONG_H_
#define _ATOMIC_LONG_H_
#include "atomic.h"

static inline void atomic_long_set(atomic_long_t *l,long i)
{
	atomic_t *v = (atomic_t *)l;

	atomic_set(v,i);
}
static inline void atomic_long_inc(atomic_long_t *l)
{
	atomic_t *v = (atomic_t *)l;

	atomic_inc(v);
}
static inline void atomic_long_dec(atomic_long_t *l)
{
	atomic_t *v = (atomic_t *)l;

	atomic_dec(v);
}
static inline void atomic_long_add(long i,atomic_long_t *l)
{
	atomic_t *v = (atomic_t *)l;

	atomic_add(i,v);
}
static inline long atomic_long_read(atomic_long_t *l)
{
	atomic_t *v = (atomic_t *)l;

	return (long)atomic_read(v);
}
#endif
