#ifndef _PERCPU_H_
#define _PERCPU_H_

#define this_cpu_ptr(ptr) (ptr)
#define this_cpu_add_return(pcp,1) ({ pcp += 1;})
#define __this_cpu_inc_return(pcp)   this_cpu_add_return(pcp,1)
#endif
