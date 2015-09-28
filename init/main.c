#include "../include/linux/memblock.h"
#include "../include/linux/debug.h"
#include <stdio.h>
#include <stdlib.h>

extern struct memblock memblock;
extern int memblock_init(void);

int main()
{

	memblock_init();
	printf("memblock memory[%p - %p]\n",
			(void *)memblock.memory.regions[0].base,
			(void *)memblock.memory.regions[0].size);

	printf("Hello World\n");
	return 0;
}
