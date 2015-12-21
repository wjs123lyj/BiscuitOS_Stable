#include "../../include/linux/kernel.h"
#include "../../include/linux/cgroup.h"


struct cgroup_subsys mem_cgroup_subsys = {
	.name = "memory",
	.disabled = 1,
};

