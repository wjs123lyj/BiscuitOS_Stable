#ifndef _PERCPU_H_
#define _PERCPU_H_

#define __this_cpu_add(pcp,val)  ({pcp += val;})

#define __this_cpu_sub(pcp,val)  __this_cpu_add((pcp),-(val))

#define this_cpu_ptr(ptr) (ptr)
#define this_cpu_dec(pcp) __this_cpu_sub((pcp),1)

#define this_cpu_add_return(pcp,1) ({ pcp += 1;})
#define __this_cpu_inc_return(pcp)   this_cpu_add_return(pcp,1)
#define __this_cpu_dec_return(pcp)   this_cpu_add_return(pcp,-1)

#define alloc_percpu(type)   \
	(typeof(type) *)__alloc_percpu(sizeof(type),__alignof__(type))

#endif
