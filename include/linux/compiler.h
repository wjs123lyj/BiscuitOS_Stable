#ifndef _COMPILER_H_
#define _COMPILER_H_    1

#define __same_type(a,b)  __builtin_types_compatible_p(typeof(a),typeof(b))
#define __acquire(x) (void)0

#define ACCESS_ONCE(x)  (*(volatile typeof(x) *)&(x))
#endif
