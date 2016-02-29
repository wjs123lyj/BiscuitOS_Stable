#include "linux/kernel.h"
#include "linux/debug.h"
#include "linux/setup.h"
#include "linux/init.h"
#include "linux/smp.h"
#include "linux/pgtable.h"
#include "linux/gfp.h"
#include "linux/page_cgroup.h"
#include "linux/mm.h"
#include "linux/kmemleak.h"
#include "linux/debugobjects.h"
#include "linux/mempolicy.h"
#include "linux/cgroup.h"
#include "linux/cpuset.h"
#include "linux/slab.h"
#include "linux/testcase.h"

extern struct task_struct init_task;
extern struct mm_struct init_mm;
#ifdef CONFIG_DEBUG_PAGEALLOC
int debug_pagealloc_enabled = 0;
#endif
extern const char linux_banner[];

enum system_states system_state __read_mostly;
/* Extern for uboot */
extern void u_boot_start(void);
/*
 * Untouced command line saved by arch-specific code.
 */
char __initdata boot_command_line[COMMAND_LINE_SIZE];

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
	kmem_cache_init();
	percpu_init_late();
	pgtable_cache_init();
	vmalloc_init();
}
void __init smp_setup_processor_id(void)
{
}

/* For debug */
extern struct obs_kernel_param __setup_early_vmalloc;
static struct obs_kernel_param *__setup_start = &__setup_early_vmalloc - 7;
static struct obs_kernel_param *__setup_end = &__setup_early_vmalloc + 2;
/* Check for early params. */
static int __init do_early_param(char *param,char *val)
{
	struct obs_kernel_param *p;
	
	for(p = __setup_start; p < __setup_end ; p++) {
		if((p->early && strcmp(param,p->str) == 0) ||
				(strcmp(param,"console") == 0 &&
				 strcmp(p->str,"earlycon") == 0)
		  ) {
			if(p->setup_func(val) != 0)
				mm_warn("Malformed early option %s\n",param);
		}
	}
	/* We accept everything at this stage. */
	return 0;
}

static inline void setup_nr_cpu_ids(void) {}
static inline void smp_prepare_cpu(unsigned int maxcpus) {}

void __init parse_early_options(char *cmdline)
{
	parse_args("early options",cmdline,NULL,0,do_early_param);
}

/* Arch code calls this early on,or if not,just before other parsing. */
void __init parse_early_param(void)
{
	static __initdata int done = 0;
	static __initdata char tmp_cmdline[COMMAND_LINE_SIZE];

	if(done)
		return;

	/* All fall through to do_early_param. */
	strcpy(tmp_cmdline,boot_command_line);
	parse_early_options(tmp_cmdline);
	done = 1;
}


static void start_kernel(void)
{
	char *command_line;

	smp_setup_processor_id();

	page_address_init();
	mm_debug("%s",linux_banner);
	setup_arch(&command_line);
	mm_init_owner(&init_mm,&init_task);
	setup_nr_cpu_ids();
	setup_per_cpu_areas();
	smp_prepare_boot_cpu();
	
	build_all_zonelists(NULL);
	page_alloc_init();

	mm_debug("Kernel command line:%s\n",boot_command_line);
	parse_early_param();
	mm_init();
	radix_tree_init();
	prio_tree_init();

	/* Interrupt are enabled now so all GFP allocations are safe. */
	gfp_allowed_mask = __GFP_BITS_MASK;

	kmem_cache_init_late();

	page_cgroup_init();
	enable_debug_pagealloc();
	kmemleak_init();
	debug_objects_mem_init();
	setup_per_cpu_pageset();
	numa_policy_init();

	anon_vma_init();
	buffer_init();
	cgroup_init();
	cpuset_init();
}

/*
 * Simulate U-boot 
 */
__attribute__((constructor)) __uboot U_boot(void)
{
	mm_debug("[%s]Boot From U_Boot\n",__FUNCTION__);
	u_boot_start();
	mm_debug("[%s]Uboot finish,start kernel...\n",__FUNCTION__);
	start_kernel();
	mm_debug("Kernel Running.....\n=====Hello World=====\n");
}
/*
 * Simulate power-down.
 */
__attribute__((destructor)) __exit Pown_down(void)
{
	mm_debug("=======Goodbye=======\n");
	mm_debug("[%s]Pown down......\n",__func__);
}

/**
 * =============================================
 *                 User Space
 * =============================================
 */
int main()
{
	TestCase_flush_all0();

	return 0;
}
