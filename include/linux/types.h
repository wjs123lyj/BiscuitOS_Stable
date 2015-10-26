#ifndef _TYPES_H_
#define _TYPES_H_

#include "page.h"

typedef unsigned int phys_addr_t;
typedef unsigned int mem_addr_t;

/*
 * Use unsigned int to act physical memory.
 */

/* Define total bytes of X_M */
#define SZ_1M   (unsigned int)(1UL << 20)
#define SZ_2M   (unsigned int)(SZ_1M << 1)
#define SZ_4M   (unsigned int)(SZ_2M << 1)
#define SZ_8M   (unsigned int)(SZ_4M << 1)
#define SZ_16M  (unsigned int)(SZ_8M << 1)
#define SZ_32M  (unsigned int)(SZ_16M << 1)
#define SZ_64M  (unsigned int)(SZ_32M << 1)
#define SZ_128M (unsigned int)(SZ_64M << 1)
#define SZ_256M (unsigned int)(SZ_128M << 1)
#define SZ_512M (unsigned int)(SZ_256M << 1)
/* Define total bytes of X_G */
#define SZ_1G   (unsigned int)(SZ_512M << 1)
#define SZ_2G   (unsigned int)(SZ_1G << 1)
#define SZ_4G   (unsigned int)(SZ_2G << 1)
#define SZ_8G   (unsigned int)(SZ_4G << 1)
/* Define total bytes of X_K */
#define SZ_1K   (unsigned int)(1UL << 10)
#define SZ_2K   (unsigned int)(SZ_1K << 1)
#define SZ_4K   (unsigned int)(SZ_2K << 1)
#define SZ_8K   (unsigned int)(SZ_4K << 1)
#define SZ_16K  (unsigned int)(SZ_8K << 1)
#define SZ_32K  (unsigned int)(SZ_16K << 1)
#define SZ_64K  (unsigned int)(SZ_32K << 1)
#define SZ_128K (unsigned int)(SZ_64K << 1)
#define SZ_256K (unsigned int)(SZ_128K << 1)
#define SZ_512K (unsigned int)(SZ_256K << 1)


#endif
