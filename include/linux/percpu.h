#ifndef _PERCPU_H_
#define _PERCPU_H_
#include "list.h"
#include "kernel.h"

struct pcpu_chunk {
	struct list_head list; /* Linked to pcpu_slot lists */
	int free_size;   /* free bytes in the chunk */
	int contig_hint; /* Max contiguous size hit */
	void *base_addr; /* Base address of this chunk */
	int map_used;  /* # of map entries used */
	int map_alloc; /* # of map entries allocated */
	int *map;  /* allocation map */
	void *data;  /* chunk data */
	bool immutable; /* no[de] population allowed */
	unsigned long populated[]; /* populated bitmap */
};

#define __this_cpu_add(pcp,val)  ({pcp += val;})

#define __this_cpu_sub(pcp,val)  __this_cpu_add((pcp),-(val))

#define this_cpu_ptr(ptr) (ptr)
#define this_cpu_dec(pcp) __this_cpu_sub((pcp),1)

#define this_cpu_add_return(pcp,val) ({ pcp += 1;})
#define __this_cpu_inc_return(pcp)   this_cpu_add_return(pcp,1)
#define __this_cpu_dec_return(pcp)   this_cpu_add_return(pcp,-1)

#define alloc_percpu(type)   \
	(typeof(type) *)__alloc_percpu(sizeof(type),__alignof__(type))

#define __this_cpu_sub(pcp,val) __this_cpu_add((pcp),-(val))
#define __this_cpu_dec(pcp)     __this_cpu_sub((pcp),1)

extern int pcpu_unit_pages __read_mostly;
extern int pcpu_unit_size  __read_mostly;
extern int pcpu_nr_units __read_mostly;
extern int pcpu_atom_size __read_mostly;
extern int pcpu_nr_slots __read_mostly;
extern int pcpu_chunk_struct_size __read_mostly;
extern int pcpu_first_unit_cpu __read_mostly;
extern int pcpu_last_unit_cpu  __read_mostly;

extern const unsigned long *pcpu_unit_offsets __read_mostly;
extern int pcpu_nr_groups __read_mostly;
extern const unsigned long *pcpu_group_offsets __read_mostly;
extern size_t *pcpu_group_sizes __read_mostly;




#endif
