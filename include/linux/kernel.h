#ifndef _KERNEL_H_
#define _KERNEL_H_
#include "config.h"

#define __init
#define __init_memblock
#define __initdata
#define __meminitdata
#define __meminit
#define __paginginit
#define __init_refok

#define max(x,y) ({ \
		typeof(x) __x = x; \
		typeof(y) __y = y; \
		__x > __y ? x : y; \
		})

#define min(x,y) ({ \
		typeof(x) __x = x; \
		typeof(y) __y = y; \
		__x > __y ? y : x; \
		})

#define ALIGN(x,align) ({ \
		((x) & ~((align) - 1)); \
		})

#define roundup(x,y)  (            \
		{                          \
		const typeof(y) __y = y;   \
		(((x) + (__y - 1)) / __y) * __y; \
		}                          \
		)

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({			\
			const typeof(((type *)0)->member) * __mptr = (ptr);	\
			(type *)((char *)__mptr - offsetof(type, member)); })

#define DIV_ROUND_UP(n,d)   (((n) + (d) - 1) / (d))

#ifdef CONFIG_NUMA
#define NUMA_BUILD  1
#else
#define NUMA_BUILD  0
#endif

#define likely(x)   (x)
#define unlikely(x)   (x)
#endif
