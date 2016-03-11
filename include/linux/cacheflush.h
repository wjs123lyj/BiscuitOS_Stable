#ifndef _CACHEFLUSH_H_
#define _CACHEFLUSH_H_    1

#define flush_cache_vunmap(x,y) do {} while(0)
#define flush_cache_vmap(x,y) do {} while(0)
/*
 * Perform necessary cache operations to ensure that the TLB will 
 * see data written in the specified area.
 */
#define clean_dcache_area(start,size) memset(start,0,size)
#define __flush_dcache_page(x,p)        do {} while(0)
#endif
