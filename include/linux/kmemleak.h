#ifndef _KMEMLEAK_H_
#define _KMEMLEAK_H_

static inline void kmemleak_alloc_recursive(const void *ptr,size_t size,
		int min_count,unsigned long flags,gfp_t gfp)
{
}
static inline void kmemleak_alloc(const void *ptr,size_t size,
		int min_count,gfp_t gfp)
{
}
static inline void kmemleak_free(const void *ptr)
{
}
#endif