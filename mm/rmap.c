#include "linux/kernel.h"
#include "linux/slab.h"
#include "linux/rmap.h"
#include "linux/slub_def.h"
#include "linux/spinlock.h"
#include "linux/list.h"

static struct kmem_cache *anon_vma_cachep;
static struct kmem_cache *anon_vma_chain_cachep;


static void anon_vma_ctor(void *data)
{
	struct anon_vma *anon_vma = data;

	spin_lock_init(&anon_vma->lock);
	anonvma_external_refcount_init(anon_vma);
	INIT_LIST_HEAD(&anon_vma->head);
}

void __init anon_vma_init(void)
{
	anon_vma_cachep = kmem_cache_create("anon_vma",
			sizeof(struct anon_vma),0,
			SLAB_DESTROY_BY_RCU | SLAB_PANIC,anon_vma_ctor);
	anon_vma_chain_cachep = KMEM_CACHE(anon_vma_chain,SLAB_PANIC);
}
