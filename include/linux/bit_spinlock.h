#ifndef _BIT_SPINLOCK_H_
#define _BIT_SPINLOCK_H_
#include "linux/compiler.h"
#include "linux/lock.h"

#define bitlock 1
/*
 * Return ture if it was acquired.
 */
static inline bit_spin_trylock(int bitnum,unsigned long *addr)
{
#if defined(CONFIG_SMP) || defined(CONFIG_DEBUG_SPINLOCK)
	if(unlikely(test_and_set_bit_lock(bitnum,
					(unsigned int *)(unsigned long)addr)))
		return 0;
#endif
	return 1;
}

/*
 * bit-based spin_lock()
 *
 * Don't use the unless you really need to:spin_lock() and spin_unlock()
 * are significantly faster.
 */
static inline void bit_spin_lock(int bitnum,unsigned long *addr)
{
	/*
	 * Assuming the lock is uncontended,this never enters
	 * the body of the outer loop.If it is contended,then
	 * within the inner loop a non-atomic test is used to
	 * busywaut with less bus contention for a good time to 
	 * attempt to qcquire the lock bit.
	 */
	__acquire(bitlock);

}


#endif
