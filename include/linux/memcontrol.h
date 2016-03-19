#ifndef _MEMCONTROL_H_
#define _MEMCONTROL_H_

#include "mm_types.h"
#include "cgroup.h"

extern struct cgroup_subsys mem_cgroup_subsys;

static inline bool mem_cgroup_disabled(void)
{
	if(mem_cgroup_subsys.disabled)
		return true;
	return false;
}

static inline void mem_cgroup_del_lru_list(struct page *page,int lru)
{
	return;
}

static inline int mem_cgroup_cache_charge(struct page *page,
		struct mm_struct *mm,gfp_t gfp_mask)
{
	return 0;
}

static inline void mem_cgroup_uncharge_cache_page(struct page *page)
{
}

#endif
