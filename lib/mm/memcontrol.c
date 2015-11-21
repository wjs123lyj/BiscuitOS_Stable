#include "../../include/linux/cgroup.h"

struct cgroup_subsys mem_cgroup_subsys = {
	.name = "memory",
	.disabled = 1,
};

/*
 * The memory controller data structure.
 */
struct mem_cgroup {
	int last_scanned_child;
};
