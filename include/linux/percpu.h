#ifndef _PERCPU_H_
#define _PERCPU_H_

#include "list.h"
#include "pfn.h"

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
	unsigned int populated[]; /* populated bitmap */
};

#define __this_cpu_add(pcp,val)  ({pcp += val;})

#define __this_cpu_sub(pcp,val)  __this_cpu_add((pcp),-(val))

#define __this_cpu_read(pcp)   (pcp)

#define this_cpu_ptr(ptr) (ptr)
#define this_cpu_dec(pcp) __this_cpu_sub((pcp),1)

#define __this_cpu_ptr(ptr) this_cpu_ptr(ptr)

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
extern unsigned int pcpu_first_unit_cpu __read_mostly;
extern unsigned int pcpu_last_unit_cpu  __read_mostly;

extern const unsigned long *pcpu_unit_offsets __read_mostly;
extern int pcpu_nr_groups __read_mostly;
extern const unsigned long *pcpu_group_offsets __read_mostly;
extern size_t *pcpu_group_sizes __read_mostly;


/* minimum unit size,also is the maximum supported allocation size */
#define PCPU_MIN_UNIT_SIZE      PFN_ALIGN(32 << 10)

/*
 * Percpu allocator can serve percpu allocations before slab is
 * initialized which allows slab to depend on the percpu allocator.
 * The following two parameters decide how much resource to 
 * preallocate for this.Keep PERCPU_DYNAMIC_RESERVE equal to or
 * larger than PERCPU_DYNAMIC_ERALY_SIZE.
 */
#define PERCPU_DYNAMIC_EARLY_SLOTS  128
#define PERCPU_DYNAMIC_EARLY_SIZE   (12 << 10)

#define per_cpu_ptr(ptr,cpu) (ptr)

#endif
