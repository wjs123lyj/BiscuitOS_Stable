#include "../../include/linux/slub_def.h"
#include "../../include/linux/kernel.h"
#include "../../include/linux/nodemask.hi"
#include "../../include/linux/page.h"

#include "../../include/asm/cache.h"

static int kmem_size = sizeof(struct kmem_cache);
static struct kmem_cache *kmem_cache;

void __init kmem_cache_init(void)
{
	int i;
	int caches = 0;
	struct kmem_cache *temp_kmem_cache;
	int order;
	struct kmem_cache *tmp_kmem_cache_node;
	unsigned long kmalloc_size;

	kmem_size = offset(struct kmem_cache,node) +
		nr_node_ids * sizeof(struct kmem_cache_node *);

	/*
	 * Allocate two kmem_caches from the page allocator.
	 */
	kmalloc_size = ALIGN(kmem_size,cache_line_size());
	order = get_order(2 * kmalloc_size);
	kmem_cache = (void *)(unsigned long)__get_free_pages(GFP_NOWAIT,order);
}
