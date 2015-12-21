#ifndef _NODEMASK_H_
#define _NODEMASK_H_

#include "bitops.h"
#include "numa.h"

typedef struct {DECLARE_BITMAP(bits,MAX_NUMNODES);} nodemask_t;

/*
 * Bitmasks that are kept for all the nodes.
 */
enum node_states {
	N_POSSIBLE, /* The node could become online at some point */
	N_ONLINE,   /* The node is online */
	N_NORMAL_MEMORY, /* The node has regular memory */
#ifdef CONFIG_HIGHMEM
	N_HIGH_MEMORY,
#endif
	N_CPU, /* The node has one or more cpus */
	NR_NODE_STATES,
};

extern nodemask_t node_states[NR_NODE_STATES];

#define NODE_MASK_LAST_WORD BITMAP_LAST_WORD_MASK(MAX_NUMNODES)

#define NODE_MASK_ALL          \
	((nodemask_t) {{             \
	 [BITS_TO_LONGS(MAX_NUMNODES) - 1] = NODE_MASK_LAST_WORD      \
	 }})
#define nr_online_nodes        1
#define first_online_node      0

#define next_online_node(nid)  (MAX_NUMNODES)

#define for_each_node_state(node,__state) \
	for((node) = 0 ; (node) == 0 ; (node) = 1)

static inline int node_state(int node,enum node_states state)
{
	return node == 0;
}
static inline void node_set_state(int node,enum node_states state)
{
}

#define node_online(node)      node_state((node),N_ONLINE)
#define node_set_online(node)  node_set_state((node),N_ONLINE)

#define for_each_online_node(node) for_each_node_state(node,N_ONLINE)

#define for_each_zone_zonelist_nodemask(zone,z,zlist,highidx,nodemask) \
	for(z = first_zones_zonelist(zlist,highidx,nodemask,&zone);  \
			zone;     \
			z = next_zones_zonelist(++z,highidx,nodemask,&zone))


#define nr_node_ids 1
#endif
