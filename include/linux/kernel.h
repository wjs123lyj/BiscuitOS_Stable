#ifndef _KERNEL_H_
#define _KERNEL_H_


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

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({			\
			const typeof(((type *)0)->member) * __mptr = (ptr);	\
			(type *)((char *)__mptr - offsetof(type, member)); })
#endif
