#include "../include/linux/memblock.h"
#include "../include/linux/debug.h"
#include "../include/linux/kernel.h"
#include <stdio.h>
#include <stdlib.h>

//extern void arch_setup(void);
extern struct memory memory;
extern struct memblock memblock;
extern void arch_init(void);
extern int test_and_set_bit(int,volatile unsigned long *);

int main()
{

//	unsigned long min,max;
	unsigned long max[5] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
	int i;
	int err;

//	arch_setup();
	arch_init();
	printf("Main[%p - %p]\n",(void *)memory.start,(void *)memory.end);
	printf("Main Curent limit %p\n",(void *)memblock.current_limit);
	printf("Main Memory cnt:%ld max %ld\n",memblock.memory.cnt,memblock.memory.max);
	printf("max %p\n",(void *)max);
	printf("Hello World\n");
	return 0;
}
