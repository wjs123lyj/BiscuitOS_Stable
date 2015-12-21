#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#define spin_lock(x)   do {} while(0)
#define spin_unlock(x) do {} while(0)
#define spin_lock_init(x) do {} while(0)
#define lock_irq_save(x)  do {} while(0)
#define lock_irq_restore(x)  do {} while(0)
#define spin_lock_irqsave(x,y) do {} while(0)
#define spin_unlock_irqrestore(x,y) do {} while(0)
#define spin_trylock(x)   (1)

#endif
