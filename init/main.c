//#include "../include/linux/memblock.h"
#include "../include/linux/debug.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
	unsigned long min,max_low;

	calculate_limit(&min,&max_low);
	printf("min %p max_low %p\n",(void *)min,(void *)max_low);

	printf("Hello World\n");
	return 0;
}
