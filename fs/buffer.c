#include "linux/kernel.h"
#include "linux/slab.h"
#include "linux/buffer_head.h"
#include "linux/page.h"
#include "linux/cpu.h"
#include "linux/notifier.h"

/*
 * Buffer-head allocation.
 */
static struct kmem_cache *bh_cachep;

/*
 * Once the number of bh's in the machine exceeds this level,we start
 * stripping them in writeback.
 */
static int max_buffer_heads;

static void buffer_exit_cpu(int cpu)
{
}

static int buffer_cpu_notify(struct notifier_block *self,
		unsigned long action,void *hcpu)
{
	if(action == CPU_DEAD || action == CPU_DEAD_FROZEN)
		buffer_exit_cpu((unsigned long)hcpu);
	return NOTIFY_OK;
}

void __init buffer_init(void)
{
	int nrpages;

	bh_cachep = kmem_cache_create("buffer_head",
			sizeof(struct buffer_head),0,
			(SLAB_RECLAIM_ACCOUNT | SLAB_PANIC |
			 SLAB_MEM_SPREAD),
			NULL);

	/*
	 * Limit the bh occupancy to 10% of ZONE_NORMAL
	 */
	nrpages = (nr_free_buffer_pages() * 10) / 100;
	max_buffer_heads = nrpages * (PAGE_SIZE / sizeof(struct buffer_head));
	hotcpu_notifier(buffer_cpu_notify,0);
}
