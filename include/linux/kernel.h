#ifndef _KERNEL_H_
#define _KERNEL_H_

#ifdef _UNUSE_STD_

#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "autoconf.h"
#include "compiler.h"
#include "types.h"

#define UINT_MAX (~0U)

#define true  1
#define false 0
#define bool   int

/* Values used for system_state */
enum system_states {
	SYSTEM_BOOTING,
	SYSTEM_RUNNING,
	SYSTEM_HALT,
	SYSTEM_POWER_OFF,
	SYSTEM_RESTART,
	SYSTEM_SUSPEND_DISK,
} system_state;

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

#define __ALIGN_KERNEL_MASK(x,mask)    (((x) + (mask)) & ~(mask))
#define __ALIGN_KERNEL(x,a)    __ALIGN_KERNEL_MASK(x,(typeof(x))(a) - 1)

#define ALIGN(x,a)       __ALIGN_KERNEL((x),(a))
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

#define likely(x)     (x)
#define unlikely(x)   (x)

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define might_sleep_if(cond) do {} while(0)

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

extern const char hex_asc[];
#define hex_asc_lo(x)  hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x)  hex_asc[((x) & 0xf0) >> 4]
static inline char *pack_hex_byte(char *buf,u8 byte)
{
	*buf++ = hex_asc_hi(byte);
	*buf++ = hex_asc_lo(byte);
	return buf;
}

/*
 * abs() handles unsigned and singed longs,inits,shorts and chars.For all
 * input types abs() returns a signed long.
 * abs() should not be used for 64-bit types(s64,u64,long long) - use abs64()
 * for those.
 */
#define abs(x) ({                   \
		long ret;                      \
		if(sizeof(x) == sizeof(long)) {           \
			long __x = (x);                       \
			ret = (__x < 0) ? -__x : __x;         \
		} else {                                  \
			int __x = (x);                        \
			ret = (__x < 0) ? -__x : __x;         \
		}                                         \
		ret;                                      \
		})


#endif
