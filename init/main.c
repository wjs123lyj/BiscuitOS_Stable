#include "../include/linux/setup.h"
#include "../include/linux/init_mm.h"
#include "../include/linux/init_task.h"
#include "../include/linux/debug.h"
#include "../include/linux/bitops.h"
#include "../include/linux/page-flags.h"
#include "../include/linux/highmem.h"
#include "../include/linux/page.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef CONFIG_DEBUG_PAGEALLOC
int debug_pagealloc_enabled = 0;
#endif
/*
 * Set up kernel memory allocators.
 */
static void __init mm_init(void)
{
	/*
	 * page_cgroup requires continues pages as memmap
	 * and it's bigger than MAX_ORDER unless SPARSEMEM.
	 */	
	page_cgroup_init_flatmem();
	mem_init();
}
void __init smp_setup_processor_id(void)
{
}

static void start_kernel(void)
{
	char *command_line;

	smp_setup_processor_id();

	setup_arch();
	mm_init_owner(&init_mm,&init_task);
	build_all_zonelist(NULL);
	page_alloc_init();
	mm_init();
}

int main()
{

	start_kernel();

	printf("Hello World\n");
	return 0;
}
