#include "../../include/linux/memblock.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/kernel.h"
#include "../../include/linux/list.h"
#include "../../include/linux/pgtable.h"
#include "../../include/linux/mm_type.h"
#include "../../include/linux/types.h"
#include "../../include/linux/highmem.h"
#include "../../include/linux/config.h"
#include "../../include/linux/boot_arch.h"
#include "../../include/linux/init_mm.h"
#include "../../include/linux/mmu.h"
#include "../../include/linux/pgalloc.h"
#include "../../include/linux/highmem.h"
#include "../../include/linux/setup.h"
#include "../../include/linux/ioport.h"
#include "../../include/linux/resource.h"

#include "../../include/asm/sections.h"
#include "../../include/asm/arch.h"
#include "../../include/asm/system.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Standard memory resource.
 */
static struct resource mem_res[] = {
	{
		.name   = "Video RAM",
		.start  = 0,
		.end    = 0,
		.flags  = IORESOURCE_MEM
	},
	{
		.name   = "Kernel text",
		.start  = 0,
		.end    = 0,
		.flags  = IORESOURCE_MEM
	},
	{
		.name   = "Kernel data",
		.start  = 0,
		.end    = 0,
		.flags  = IORESOURCE_MEM
	}
};
#define video_ram    mem_res[0]
#define kernel_code  mem_res[1]
#define kernel_data  mem_res[2]

static struct resource io_res[] = {
	{
		.name    = "reserved",
		.start   = 0x3bc,
		.end     = 0x3be,
		.flags   = IORESOURCE_IO | IORESOURCE_BUSY
	},
	{
		.name    = "reserved",
		.start   = 0x378,
		.end     = 0x27f,
		.flags   = IORESOURCE_IO | IORESOURCE_BUSY
	},
	{
		.name    = "reserved",
		.start   = 0x278,
		.end     = 0x27f,
		.flags   = IORESOURCE_IO | IORESOURCE_BUSY
	}
};

#define lp0    io_res[0]
#define lp1    io_res[1]
#define lp2    io_res[2]

struct machine_desc buddy_machine = {
	.nr   = 0x20103201,
	.name = "Buddy allocator",
};
#define machine_arch_type  0x20103201
/*
 * lookup machine type
 */
struct machine_desc *lookup_machine_type(unsigned int nr)
{
	return &buddy_machine;
}
static inline void reserve_crashkernel(void) {}
static void __init request_standard_resource(struct machine_desc *mdesc)
{
	struct memblock_region *region;
	struct resource *res;

	kernel_code.start  = (unsigned long)__executable_start;
	kernel_code.end    = (unsigned long)(_etext - 1);
	kernel_data.start  = (unsigned long)_edata;
	kernel_data.end    = (unsigned long)(_end - 1);

	for_each_memblock(memory,region)
	{
		res = (struct resource *)(unsigned long)alloc_bootmem_low(sizeof(*res));
		res->name = "SystemRAM";
		res->start = __pfn_to_phys(memblock_region_memory_base_pfn(region));
		res->end   = __pfn_to_phys(memblock_region_memory_end_pfn(region));
		res->flags = IORESOURCE_MEM | IORESOURCE_BUSY;

		request_resource(&iomem_resource,res);

		if(kernel_code.start >= res->start &&
				kernel_code.end <= res->end)
			request_resource(res,&kernel_code);
		if(kernel_code.start >= res->start &&
				kernel_code.end <= res->end)
			request_resource(res,&kernel_data);
	}
	if(mdesc->video_start)
	{
		video_ram.start = mdesc->video_start;
		video_ram.end   = mdesc->video_end;
		request_resource(&iomem_resource,&video_ram);
	}
	/*
	 * Some machines don't have the possiblilty of ever
	 * possessing lp0,lp1 or lp2.
	 */
	if(mdesc->reserve_lp0)
		request_resource(&ioport_resource,&lp0);
	if(mdesc->reserve_lp1)
		request_resource(&ioport_resource,&lp1);
	if(mdesc->reserve_lp2)
		request_resource(&ioport_resource,&lp2);
}
/*
 * setup machine
 */
static struct machine_desc * __init setup_machine(unsigned int nr)
{
	struct machine_desc *list;

	/*
	 * locate machine in the list of supported machines.
	 */
	list = lookup_machine_type(nr);
	if(!list)
	{
		mm_debug("Machine configuration botched(nr %d),unable "
				"to continue.\n",nr);
		while(1);
	}
	mm_debug("Machine %s\n",list->name);

	return list;
}
/*
 * setup_arch
 */
void __init setup_arch(void)
{

	struct machine_desc *mdesc;

	unwind_init();

	mdesc = setup_machine(machine_arch_type);

	init_mm.start_code  = (unsigned long)__executable_start;
	init_mm.end_code    = (unsigned long)_etext;
	init_mm.end_data    = (unsigned long)_edata;
	init_mm.brk         = (unsigned long)_end;
	/* Initialize virtual physical layer */
	virt_arch_init();
	/* Early parment */
	early_parment();
	/* ARM init memblock */
	arm_memblock_init(&meminfo);
	/* Initlize the paging table */
	paging_init();
	/* Request standard resource */
//	request_standard_resource(mdesc);

	reserve_crashkernel();
	/* User debug */
	/* End debug */
}

int cpu_architecture(void)
{
	return CPU_ARCH_ARMv7;
}
