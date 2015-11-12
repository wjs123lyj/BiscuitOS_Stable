#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#define ATOMIC_INIT(i)    {(i)}
#define atomic_read(v)    (*(volatile int *)&(v)->counter)
#define atomic_set(v,i)   (((v)->counter) = i)

#endif
