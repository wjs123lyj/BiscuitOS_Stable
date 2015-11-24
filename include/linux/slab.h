#ifndef _SLAB_H_
#define _SLAB_H_

/*
 * Flags to pass to kmem_cache_create().
 * The one marks DEBUG are only valid if CONFIG_SLAB_DEBUG is set.
 */
#define SLAB_DEBUG_FREE     0x00000100UL /* DEBUG:Perform checks on free */
#define SLAB_RED_ZONE       0x00000400UL /* DEBUG:Red zone onjs in a cache */
#define SLAB_POISON         0x00000800UL /* DEBUG:Poison objects */
#define SLAB_HWCACHE_ALIGN  0x00002000UL /* DEBUG:Align objs on cache lines */
#define SLAB_STORE_USER     0x00010000UL /* DEBUG:Store the last owner for bug*/
#define SLAB_PANIC          0x00040000UL /* DEBUG:Panic */

/*
 * See also the comment on struct slab_rcu mm/slab.c
 */
#define SLAB_DESTROT_BY_RCU  0x00080000UL /* Defer freeing slabs to RCU */ 
#define SLAB_MEM_SPREAD      0x00100000UL /* Spread some memory over cpuset */
#define SLAB_TARCE           0x00200000UL /* Trace allocations and frees */


/*
 * Don't track use of uninitialized memory.
 */
#define SLAB_NOTRACK   0x00000000UL

static inline void *kmem_cache_alloc_node(struct kmem_cache *cachep,
		gfp_t flags,int node)
{
	return kmem_cache_alloc(cachep,flags);
}
#endif
