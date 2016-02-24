#ifndef _PAGE_CGROUP_H_
#define _PAGE_CGROUP_H_

#include "mem_cgroup.h"
#include "mm_types.h"
#include "list.h"
/*
 * Page Cgoup can be considered as an extended mem_map.
 * A page_cgroup is associated with every page descriptor.The
 * page_cgroup helps us identify information about the cgroup
 * All page cgroup are allocated at boot or memory hotplug event,
 * then the page cgroup for pfn always exists.
 */
struct page_cgroup {
	unsigned long flags;
	struct mem_cgroup *mem_cgroup;
	struct page *page;
	struct list_head lru; /* per cgroup LRU list */
};

static inline void __init page_cgroup_init(void)
{
}

#endif
