#ifndef _DEBUG_LOCKS_H_
#define _DEBUG_LOCKS_H_

static inline void debug_check_no_locks_freed(const void *from,
		unsigned long len)
{
}
static inline void debug_check_no_obj_freed(const void *address,
		unsigned long size)
{
}
#endif
