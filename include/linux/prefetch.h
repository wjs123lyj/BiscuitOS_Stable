#ifndef _PREFETCH_H_
#define _PREFETCH_H_

#define prefetchw(x)  __builtin_prefetch(x,1)
#define prefetch(x)   __builtin_prefetch(x)
#endif
