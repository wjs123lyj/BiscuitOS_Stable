#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#define ATOMIC_INIT(i)    {(i)}
#define atomic_read(v)    (*(volatile int *)&(v)->counter)
#define atomic_set(v,i)   (((v)->counter) = i)

static inline int atomic_add_return(int i,atomic_t *v)
{
	int ret;

	ret = v->counter += i;

	return ret;
}
static inline int atomic_sub_return(int i,atomic_t *v)
{
	int ret;

	ret = v->counter -= i;

	return ret;
}
static inline int atomic_cmpxchg(atomic_t *ptr,int old,int new)
{
	return 1;
}
static inline int atomic_add_unless(atomic_t *v,int a,int u)
{
	int c,old;

	c = atomic_read(v);
	while(c != u && (old = atomic_cmpxchg((v),c, c + a)) != c)
		c = old;
	return c != u;
}

#define atomic_inc_not_zero(v)  atomic_add_unless((v),1,0)
#define atomic_dec_and_test(v)  (atomic_sub_return(1,v) == 0)
#define atomic_add(i,v)   ((void)atomic_add_return((i),(v)))
#define atomic_sub(i,v)   ((void)atomic_sub_return((i),(v)))

#define atomic_dec_and_test(v)  (atomic_sub_return(1,(v)) == 0)

#define atomic_inc(v)     atomic_add(1,v)
#define atomic_dec(v)     atomic_sub(1,v)
#endif
