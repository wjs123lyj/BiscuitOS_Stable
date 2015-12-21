#ifndef _CGROUP_H_
#define _CGROUP_H_   1

#include "list.h"

struct cgroup_subsys {
	const char *name;
	int disabled;
	int active;
	int subsys_id;
	struct list_head sibling;
};

#endif
