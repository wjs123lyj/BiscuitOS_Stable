#ifndef _DEBUG_LOCKS_H_
#define _DEBUG_LOCKS_H_   1

static inline void debug_check_no_locks_freed(void *from,
		unsigned long len)
{
}
static inline void debug_check_no_obj_freed(void *address,
		unsigned long size)
{
}
#endif
