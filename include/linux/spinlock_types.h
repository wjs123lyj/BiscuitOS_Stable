#ifndef _SPINLOCK_TYPES_H_
#define _SPINLOCK_TYPES_H_

typedef struct {
	volatile unsigned int lock;
} arch_spinlock_t;

typedef struct raw_spinlock {
	arch_spinlock_t raw_lock;
} raw_spinlock_t;

typedef struct spinlock {
	union {
		struct raw_spinlock rlock;
	}
} spinlock_t;


#define __ARCH_SPIN_LOCK_UNLOCKED {0}

typedef struct {
	volatile unsigned int lock;
} arch_rwlock_t;

#define __ARCH_RW_LOCK_UNLOCKED    {0}
#define __ARCH_SPIN_LOCK_UNLOCKED  {0}

#define __RAW_SPIN_LOCK_INITIALIZER(lockname) \
		{             \
			.raw_lock = __ARCH_SPIN_LOCK_UNLOCKED,  \
		}

#define __SPIN_LOCK_INITIALIZE(lockname) \
		{{ .rlock = __RAW_SPIN_LOCK_INITIALIZER(lockname) }}

#define __SPIN_LOCK_UNLOCKED(lockname)  \
	(spinlock_t)__SPIN_LOCK_INITIALIZE(lockname)

#define DEFINE_SPINLOCK(x) spinlock_t x = __SPIN_LOCK_UNLOCKED(x)
#endif
