#ifndef _CPUSET_H_
#define _CPUSET_H_   1
#include "nodemask.h"
#include "mmzone.h"

#define cpuset_current_mems_allowed (node_states[N_HIGH_MEMORY])
static inline void get_mems_allowed(void)
{
}
static inline void put_mems_allowed(void)
{
}
static inline int cpuset_zone_allowed_softwall(struct zone *z,gfp_t gfp_mask)
{
	return 1;
}
static inline int cpuset_zone_allowed_hardwall(struct zone *z,gfp_t gfp_mask)
{
	return 1;
}
static inline void cpuset_memory_pressure_bump(void) {}

static inline void cpuset_init_current_mems_allowed(void) {}

static inline int cpuset_init(void) {return 0;}
#endif
