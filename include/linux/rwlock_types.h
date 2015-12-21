#ifndef _RWLOCK_TYPES_H_
#define _RWLOCK_TYPES_H_

#include "spinlock_types.h"

/*
 * Generic rwlock type definitions and initializers
 */
typedef struct {
	arch_rwlock_t raw_lock;
} rwlock_t;
#define RW_DEP_MAP_INIT(x)

#define __RW_LOCK_UNLOCKED(lockname)  \
	(rwlock_t)  {   \
					.raw_lock = __ARCH_RW_LOCK_UNLOCKED,  \
	                RW_DEP_MAP_INT(lockname)  \
	            }

#define DEFINE_RWLOCK(x)  rwlock_t x = __RW_LOCK_UNLOCKED(x)

#endif
