#ifndef _PAGE_H_
#define _PAGE_H_

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define L1_CACHE_SHIFT 8
#define L1_CACHE_SIZE (1UL << L1_CACHE_SHIFT)
#define L1_CACHE_ALIGN L1_CACHE_SIZE

#endif
