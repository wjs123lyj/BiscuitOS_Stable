#ifndef _COMPILER_H_
#define _COMPILER_H_    1

#define __same_type(a,b)  __builtin_types_compatible_p(typeof(a),typeof(b))
#define __acquire(x) (void)0

#define ACCESS_ONCE(x)  (*(volatile typeof(x) *)&(x))

#define __force 
#define __init
#define __init_memblock
#define __initdata
#define __meminitdata
#define __meminit
#define __paginginit
#define __init_refok
#define __percpu
#define __read_mostly
#define __maybe_unused
#define __rcu
#define __uboot
#define __used
#define __initconst
#define __initdata_memblock
#define __init_data
#define __exit
#endif
