#ifndef _TYPES_H_
#define _TYPES_H_


typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char     s8;
typedef short           s16;
typedef int             s32;
typedef long long       s64;

typedef unsigned int phys_addr_t;
typedef unsigned int mem_addr_t;
typedef unsigned int gfp_t;
typedef struct {
	int counter;
} atomic_t;
typedef atomic_t atomic_long_t;

#define pgoff_t unsigned long

typedef phys_addr_t resource_size_t;

typedef unsigned long __kernel_size_t;

typedef __kernel_size_t size_t;

/*
 * Use unsigned int to act physical memory.
 */

/* Define total bytes of X_M */
#define SZ_1M   (unsigned long)(1UL << 20)
#define SZ_2M   (unsigned long)(SZ_1M << 1)
#define SZ_4M   (unsigned long)(SZ_2M << 1)
#define SZ_8M   (unsigned long)(SZ_4M << 1)
#define SZ_16M  (unsigned long)(SZ_8M << 1)
#define SZ_32M  (unsigned long)(SZ_16M << 1)
#define SZ_64M  (unsigned long)(SZ_32M << 1)
#define SZ_128M (unsigned long)(SZ_64M << 1)
#define SZ_256M (unsigned long)(SZ_128M << 1)
#define SZ_512M (unsigned long)(SZ_256M << 1)
/* Define total bytes of X_G */
#define SZ_1G   (unsigned long)(SZ_512M << 1)
#define SZ_2G   (unsigned long)(SZ_1G << 1)
#define SZ_4G   (unsigned long)(SZ_2G << 1)
#define SZ_8G   (unsigned long)(SZ_4G << 1)
/* Define total bytes of X_K */
#define SZ_1K   (unsigned long)(1UL << 10)
#define SZ_2K   (unsigned long)(SZ_1K << 1)
#define SZ_4K   (unsigned long)(SZ_2K << 1)
#define SZ_8K   (unsigned long)(SZ_4K << 1)
#define SZ_16K  (unsigned long)(SZ_8K << 1)
#define SZ_32K  (unsigned long)(SZ_16K << 1)
#define SZ_64K  (unsigned long)(SZ_32K << 1)
#define SZ_128K (unsigned long)(SZ_64K << 1)
#define SZ_256K (unsigned long)(SZ_128K << 1)
#define SZ_512K (unsigned long)(SZ_256K << 1)


#endif
