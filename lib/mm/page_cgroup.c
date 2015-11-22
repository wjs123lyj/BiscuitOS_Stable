#include "../../include/linux/kernel.h"
#include "../../include/linux/mmzone.h"
#include "../../include/linux/page_cgroup.h"
#include "../../include/linux/error.h"
#include "../../include/linux/mm_type.h"
#include "../../include/linux/nodemask.h"
#include "../../include/asm/smp_plat.h"
#include "../../include/linux/memcontrol.h"
#include "../../include/linux/debug.h"


static void __meminit __init_page_cgroup(struct page_cgroup *pc,
		unsigned long pfn)
{
	pc->flags = 0;
	pc->mem_cgroup = NULL;
	pc->page = pfn_to_page(pfn);
	INIT_LIST_HEAD(&pc->lru);
}
static unsigned long total_usage;

static int __init alloc_node_page_cgroup(int nid)
{
	struct page_cgroup *base,*pc;
	unsigned long table_size;
	unsigned long start_pfn,nr_pages,index;

	start_pfn = NODE_DATA(nid)->node_start_pfn;
	nr_pages  = NODE_DATA(nid)->node_spanned_pages;

	if(!nr_pages)
		return 0;

	table_size = sizeof(struct page_cgroup) * nr_pages;

	base = (struct page_cgroup *)(unsigned long)__alloc_bootmem_node_nopanic(
						NODE_DATA(nid),
			table_size,PAGE_SIZE,__pa(MAX_DMA_ADDRESS));

	if(!base)
		return -ENOMEM;
	for(index = 0 ; index < nr_pages ; index++)
	{
		pc = base + index;
		__init_page_cgroup(pc,start_pfn + index);
	}
	NODE_DATA(nid)->node_page_cgroup = base;
	total_usage += table_size;
	return 0;
}

void __init page_cgroup_init_flatmem(void)
{
	int nid,fail;

	if(mem_cgroup_disabled())
		return;

	for_each_online_node(nid)
	{
		fail = alloc_node_page_cgroup(nid);
		if(fail)
			goto fail;
	}
	mm_debug("allocated %ld bytes of page_cgroup\n",total_usage);
	mm_debug("please try cgroup_disabled=memory' option if you"
			"don't want memory cgroup\n");
	return;
fail:
	mm_err("allocation of page_cgroup failed.\n");
	mm_err("please try cgroup_disabled=memory boot operation\n");
	mm_err("Out of memory\n");
}


