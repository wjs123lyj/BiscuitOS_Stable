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

struct pcpu_group_info {
	int nr_units;  /* aligned # of units */
	unsigned long base_offset;  /* base address offset */
	unsigned int *cpu_map;  /* unit->cpu map,empty entries contain NR_CPUS */
};

struct pcpu_alloc_info {
	size_t static_size;
	size_t reserved_size;
	size_t dyn_size;
	size_t unit_size;
	size_t atom_size;
	size_t alloc_size;
	size_t __ai_size;    /* internal,don't use */
	int nr_groups;       /* 0 if grouping unnecessary */
	struct pcpu_group_info groups[];
};

enum pcpu_fc {
	PCPU_FC_AUTO,
	PCPU_FC_EMBED,
	PCPU_FC_PAGE,

	PCPU_FC_NR,
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
	(typeof(type) *)(unsigned long)__alloc_percpu(sizeof(type),  \
			__alignof__(type))

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

/*
 * Macro which verifies @ptr is a percpu pointer without evaluation
 * @ptr.This is to be used in percpu accessors to verify that the 
 * input parameter is a percpu pointer.
 */
#define __verify_pcpu_ptr(ptr)     do {    \
	const void *__vpp_verify = (typeof(ptr))NULL;   \
	(void)__vpp_verify;                              \
} while(0)

#define VERIFY_PERCPU_PTR(__p) ({     \
		__verify_pcpu_ptr((__p));        \
		(typeof(*(__p)) *)(__p);      \
})


#define per_cpu(var,cpu)    (*((void)(cpu),VERIFY_PERCPU_PTR(&(var))))

#endif
