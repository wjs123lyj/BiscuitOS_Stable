#include "../include/linux/setup.h"
#include "../include/linux/init_mm.h"
#include "../include/linux/init_task.h"
#include "../include/linux/debug.h"
#include "../include/linux/bitops.h"
#include <stdio.h>
#include <stdlib.h>

//extern unsigned int top_mem;

static void start_kernel(void)
{
	setup_arch();
	mm_init_owner(&init_mm,&init_task);
	build_all_zonelist(NULL);
}

int main()
{

	start_kernel();

	printf("Hello World\n");
	return 0;
}
