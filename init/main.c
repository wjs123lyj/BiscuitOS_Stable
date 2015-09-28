#include "../include/linux/memblock.h"
#include "../include/linux/debug.h"
#include "../include/linux/kernel.h"
#include <stdio.h>
#include <stdlib.h>

extern void arch_setup(void);

int main()
{

	unsigned long min,max;

	arch_setup();

	printf("Hello World\n");
	return 0;
}
