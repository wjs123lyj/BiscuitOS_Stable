#include "linux/kernel.h"

static void u32_swap(void *a,void *b,int size)
{
	u32 t = *(u32 *)a;
	*(u32 *)a = *(u32 *)b;
	*(u32 *)b = t;
}

static void generic_swap(void *a,void *b,int size)
{
	char t;

	do {
		t = *(char *)a;
		*(char *)a++ = *(char *)b;
		*(char *)b++ = t;
	} while(--size > 0);
}
/*
 * Sort - sort an array of elements
 *
 * This function does a heapsort on the given array.
 */
void sort(void *base,size_t num,size_t size,
		int (*cmp_func)(const void *,const void *),
		void (*swap_func)(void *,void *,int size))
{
	/* pre-scale counters for performance */
	int i = (num / 2 - 1) * size,n = num * size,c,r;

	if(!swap_func)
		swap_func = (size == 4 ? u32_swap : generic_swap);

	/* heapify */
	for( ; i >= 0 ; i -= size) {
		for(r = i ; i * 2 + size < n ; r = c) {
			c = r * 2 + size;
			if(c < n - size && 
					cmp_func(base + c,base + c + size) < 0)
				c += size;
			if(cmp_func(base + r,base + c) >= 0)
				break;
			swap_func(base + r , base + c, size);
		}
	}

	/* sort */
	for(i = n - size ; i > 0 ; i -= size) {
		swap_func(base , base + i , size);
		for(r = 0 ; r * 2 + size < i ; r = c) {
			c = r * 2 + size;
			if(c < i - size &&
					cmp_func(base + c,base + c + size) < 0)
				c += size;
			if(cmp_func(base + r,base + c) >= 0)
				break;
			swap_func(base + r, base + c , size);
		}
	}
}
