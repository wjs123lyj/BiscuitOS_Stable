#ifndef _KERNEL_H_
#define _KERNEL_H_
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "types.h"
#include <string.h>

#define __init
#define __init_memblock
#define __initdata
#define __meminitdata
#define __meminit
#define __paginginit
#define __init_refok
#define __percpu
#define __force 
#define __read_mostly
#define __maybe_unused
#define __rcu

#define true  1
#define false 0
#define bool   int


#define ULONG_MAX (~0UL)


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
#define PTR_ALIGN(p,a)   ((typeof(p))ALIGN((unsigned long)(p),(a)))
#define IS_ALIGNED(x,a)  (((x) & ((typeof(x))(a) - 1)) == 0)


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

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define might_sleep_if(cond) do {   \
	if(cond) \
		mm_debug("[Sleep]\n");}   \
		while(0)

#define _RET_IP_     (unsigned long)__builtin_return_address(0)

/*
 * and if you can't take the strict
 * types,you can specify one yourself.
 *
 * Or not use min/max/clamp at all,of course.
 */
#define min_t(type,x,y)    ({        \
		type __min1 = (x);     \
		type __min2 = (y);     \
		__min1 < __min2 ? __min1 : __min2;})

#define max_t(type,x,y)  ({     \
		type __max1 = (x);     \
		type __max2 = (y);     \
		__max1 > __max2 ? __max1 : __max2;})
/* is x a power of 2 */
#define is_power_of_2(x)   ((x) != 0 && (((x) & ((x) - 1)) == 0))

#endif
