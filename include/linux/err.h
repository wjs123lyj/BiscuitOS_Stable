#ifndef _ERR_H_
#define _ERR_H_   1

#define MAX_ERRNO 4095

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)- MAX_ERRNO)

extern void *phys_to_mem(phys_addr_t addr);
static inline void *ERR_PTR(long error)
{
	return (void *)phys_to_mem(virt_to_phys(-error));
}
static inline long PTR_ERR(const void *ptr)
{
	return (unsigned long)phys_to_virt(mem_to_phys(ptr));
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
