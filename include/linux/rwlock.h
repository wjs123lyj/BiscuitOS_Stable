#ifndef _RWLOCK_H_
#define _RWLOCK_H_

#define DEFINE_RWLOCK(x) (int x)

#define write_lock(x)    do {} while(0)
#define write_unlock(x)  do {} while(0)

#define read_lock(x)     do {} while(0)
#define read_unlock(x)   do {} while(0)

#endif
