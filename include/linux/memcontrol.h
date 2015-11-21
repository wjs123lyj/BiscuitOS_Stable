#ifndef _MEMCONTROL_H_
#define _MEMCONTROL_H_
#include "cgroup.h"

extern struct cgroup_subsys mem_cgroup_subsys;

static inline bool mem_cgroup_disabled(void)
{
	if(mem_cgroup_subsys.disabled)
		return true;
	return false;
}

#endif
