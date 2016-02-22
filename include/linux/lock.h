#ifndef _LOCK_H_
#define _LOCK_H_

extern int test_and_set_bit(int,unsigned int *);
#define test_and_set_bit_lock(nr,addr)  test_and_set_bit(nr,addr)

#endif
