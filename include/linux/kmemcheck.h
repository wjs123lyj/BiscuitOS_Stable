#ifndef _KMEMCHECK_H_
#define _KMEMCHECK_H_

#include "mm_types.h"

#define kmemcheck_enabled  0
static inline bool kmemcheck_page_is_tracked(struct page *p)
{
	return false;
}
static inline void kmemcheck_pagealloc_alloc(struct page *p,
		unsigned int order,gfp_t gfpflags)
{
}
static inline void kmemcheck_alloc_shadow(struct page *page,int order,
		gfp_t flags,int node)
{
}
static inline void kmemcheck_mark_uninitialized_pages(struct page *page,
		unsigned int n)
{
}
static inline void kmemcheck_mark_unallocated_pages(struct page *page,
		unsigned int n)
{
}
static inline void kmemcheck_slab_alloc(struct kmem_cache *s,gfp_t gfpflags,
		void *object,size_t size)
{
}
static inline void kmemcheck_free_shadow(struct page *page,int order)
{
}
#endif
