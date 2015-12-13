#ifndef _ERR_H_
#define _ERR_H_

#define MAX_ERRNO 4095

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)- MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *)error;
}
static inline long PTR_ERR(const void *ptr)
{
	return (long)ptr;
}
static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}
static inline long IS_ERR_OR_NULL(const void *ptr)
{
	return !ptr || IS_ERR_VALUE((unsigned long)ptr);
}
#endif
