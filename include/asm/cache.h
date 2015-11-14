#ifndef _CACHE_H_
#define _CACHE_H_
#include "../linux/config.h"


#define L1_CACHE_SHIFT  CONFIG_ARM_L1_CACHE_SHIFT
#define L1_CACHE_BYTES  (1UL << L1_CACHE_SHIFT)
#define L1_CACHE_SIZE   (1UL << L1_CACHE_SHIFT)
#define L1_CACHE_ALIGN  L1_CACHE_SIZE
#define SMP_CACHE_BYTES L1_CACHE_BYTES

#endif
