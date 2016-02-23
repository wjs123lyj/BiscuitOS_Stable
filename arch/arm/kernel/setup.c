#include "linux/kernel.h"
#include "asm/system.h"
#include "asm/sections.h"
#include "linux/mm_types.h"
#include "asm/arch.h"
#include "linux/memblock.h"
#include "linux/ioport.h"
#include "linux/unwind.h"
#include "linux/debug.h"
#include "linux/bootmem.h"
#include "linux/setup.h"
#include "linux/kdev_t.h"
#include "linux/fs.h"
#include "asm/errno-base.h"
#include "linux/init.h"
#include "linux/cputype.h"
#include "asm/procinfo.h"
#include "linux/cachetype.h"

extern struct meminfo meminfo;
extern struct mm_struct init_mm;
extern struct resource ioport_resource;
extern struct resource iomem_resource;
extern dev_t ROOT_DEV;
extern int root_mountflags;
extern char __initdata boot_command_line[COMMAND_LINE_SIZE];
extern void __init parse_early_param(void);
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

#ifndef MEM_SIZE
#define MEM_SIZE  (16 * 1024 * 1024)
#endif

/*
 * This holds our defaults.
 */
static struct init_tags {
	struct tag_header hdr1;
	struct tag_core core;
	struct tag_header hdr2;
	struct tag_mem32 mem;
	struct tag_header hdr3;
} init_tags __initdata = {
	{ tag_size(tag_core) , ATAG_CORE},
	{ 1 , PAGE_SIZE , 0xff},
	{ tag_size(tag_mem32) , ATAG_MEM},
	{ MEM_SIZE,PHYS_OFFSET},
	{0 , ATAG_NONE}
};

unsigned int __atags_pointer __initdata = 0;
struct machine_desc *machine_desc __initdata;
static const char *machine_name;
unsigned int cacheid __read_mostly;
static char __initdata cmd_line[COMMAND_LINE_SIZE];

static char default_command_line[COMMAND_LINE_SIZE] __initdata = 
				CONFIG_CMDLINE;
#define machine_arch_type  CONFIG_MACH_TYPE
/*
 * lookup machine type
 */
struct machine_desc *lookup_machine_type(unsigned int nr)
{
	struct machine_desc *machine;
	u32 value;

	value = get_cr1();
	machine = (struct machine_desc *)(unsigned long)value;
	
	if(nr == machine->nr) {
		/* In order to simulate boot params in RAM */
		machine->boot_params =
			(unsigned long)phys_to_mem(machine->boot_params);

		return machine;
	}
	else {
		mm_err("ERR:Can't get machine\n");
		return NULL;
	}

}
static inline void reserve_crashkernel(void) {}
static void __init request_standard_resource(struct machine_desc *mdesc)
{
	struct memblock_region *region;
	struct resource *res;
	int i = 0;

	kernel_code.start  = (unsigned long)__executable_start;
	kernel_code.end    = (unsigned long)(_etext - 1);
	kernel_data.start  = (unsigned long)_edata;
	kernel_data.end    = (unsigned long)(_end - 1);

	for_each_memblock(memory,region) {
		res = (struct resource *)(unsigned long)(
				phys_to_mem(__pa(alloc_bootmem_low(sizeof(*res)))));
		res->name = "SystemRAM";
		res->start = __pfn_to_phys(memblock_region_memory_base_pfn(region));
		res->end   = __pfn_to_phys(memblock_region_memory_end_pfn(region));
		res->flags = IORESOURCE_MEM | IORESOURCE_BUSY;
	
		request_resource(&iomem_resource,res);
		
		if(kernel_code.start >= res->start &&
				kernel_code.end <= res->end) {
			request_resource(res,&kernel_code);
		}
		if(kernel_code.start >= res->start &&
				kernel_code.end <= res->end) {
			request_resource(res,&kernel_data);
		}
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
 * Add memory
 */
static int __init arm_add_memory(unsigned long start,unsigned long size)
{
	struct membank *bank = &meminfo.bank[meminfo.nr_banks];

	if(meminfo.nr_banks >= NR_BANKS) {
		mm_debug("NR_BANKS too low,"
				"ignoring memory at %p\n",(void *)start);
		return -ENOMEM;
	}

	/*
	 * Ensure that start/size are aligned to a page boundary.
	 * Size is appropriately rounded down,start is rounded up.
	 */
	size -= start & ~PAGE_MASK;
	bank->start = PAGE_ALIGN(start);
	bank->size = size & PAGE_MASK;

	/*
	 * Check whether this memory region has non-zero size or 
	 * invalid node number.
	 */
	if(bank->size == 0)
		return -ENOMEM;

	meminfo.nr_banks++;
	return 0;
}
/*
 * Tag parsing.
 *
 * This is the new way of passing data to the kernel at boot time.Rather
 * than passing a fixed inflexible structure to the kernel,we pass a list
 * of varable-sized tags to the kernel.The first tag must be a ATAG_CORE
 * tag for the list to be recognised.The list is terminated with a 
 * zero-length tag.
 */
static int __init parse_tag_core(const struct tag *tag)
{
	if(tag->hdr.size > 2) {
		if((tag->u.core.flags & 1) == 0)
			root_mountflags &= ~MS_RDONLY;
		ROOT_DEV = old_decode_dev(tag->u.core.rootdev);
	}
	return 0;
}

__tagtable(ATAG_CORE,parse_tag_core);
/*
 * ATAG_MEM
 */
static int __init parse_tag_mem32(const struct tag *tag)
{
	return arm_add_memory(tag->u.mem.start,tag->u.mem.size);
}

__tagtable(ATAG_MEM,parse_tag_mem32);
/*
 * ATAG_CMDLINE
 */
static int __init parse_tag_cmdline(const struct tag *tag)
{
#ifndef CONFIG_CMDLINE_FORCE
	strlcpy(default_command_line,tag->u.cmdline,COMMAND_LINE_SIZE);
#else
	mm_warn("Ignoring tag cmdline(unsigned the default kernel comand line)\n");
#endif
	return 0;
}

__tagtable(ATAG_CMDLINE,parse_tag_cmdline);

static struct tagtable *__tagtable_begin = &__tagtable_parse_tag_core;
static struct tagtable *__tagtable_end = &__tagtable_parse_tag_cmdline + 1;

static void __init squash_mem_tags(struct tag *tag)
{
	for(; tag->hdr.size ; tag = tag_next(tag)) 
		if(tag->hdr.tag == ATAG_MEM) 
			tag->hdr.tag = ATAG_NONE;
}
/*
 * Scan the tag table for this tag,and call its parse function.
 * The tag table is built by the linker from all the __tagtable
 * declarations.
 */
static int __init parse_tag(const struct tag *tag)
{
	struct tagtable *t;

	for(t = __tagtable_begin; t < __tagtable_end ; t++) 
		if(tag->hdr.tag == t->tag) {
			t->parse(tag);
			break;
		}
	return t < __tagtable_end;
}
/*
 * Parse all tags in the list,checking both the global and architecture
 * specific tag tables.
 */
static void __init parse_tags(const struct tag *t)
{
	for( ; t->hdr.size ; t = tag_next(t))
		if(!parse_tag(t))
			mm_warn("Ignoring unrecognised tag %p\n",
					(void *)(unsigned long)(t->hdr.tag));
}
static int cpu_has_aliasing_icache(unsigned int arch)
{
	int aliasing_icache;
	unsigned int id_reg,num_sets,line_size;

	/* arch specifies the register format */
	switch(arch) {
		case CPU_ARCH_ARMv7:
//			C0 1d152152
			aliasing_icache = 1;
			break;
		case CPU_ARCH_ARMv6:
			aliasing_icache = read_cpuid_cachetype() & (1 << 11);
			break;
		default:
			aliasing_icache = 0;
	}
	
	return aliasing_icache;
}

static void __init cacheid_init(void)
{
	unsigned int cachetype = read_cpuid_cachetype();
	unsigned int arch = cpu_architecture();

	if(arch >= CPU_ARCH_ARMv6) {
		if((cachetype & (7 << 29)) == 4 << 29) {
			/* ARMv7 register format */
			cacheid = CACHEID_VIPT_NONALIASING;
			if((cachetype & (3 << 14)) == 1 << 14)
				cacheid |= CACHEID_ASID_TAGGED;
			else if(cpu_has_aliasing_icache(CPU_ARCH_ARMv7))
				cacheid |= CACHEID_VIPT_I_ALIASING;
		} else if(cachetype & (1 << 23)) {
			cacheid = CACHEID_VIPT_ALIASING;
		} else {
			cacheid = CACHEID_VIPT_NONALIASING;
			if(cpu_has_aliasing_icache(CPU_ARCH_ARMv6))
				cacheid |= CACHEID_VIPT_I_ALIASING;
		}
	} else {
		cacheid = CACHEID_VIVT;
	}
	
	mm_debug("CPU: %s data cache,%s instruct cache\n",
			cache_is_vivt() ? "VIVT" : 
			cache_is_vipt_aliasing() ? "VIPT aliasing" :
			cache_is_vipt_nonaliasing() ? "VIPT nonaliasing" : "unknown",
			cache_is_vivt() ? "VIVT" :
			icache_is_vivt_asid_tagged() ? "VIVT ASID tagged" :
			cache_is_vipt_nonaliasing() ? "VIPT nonliasing" : "unknown");
}
static void __init setup_processor(void)
{
#ifdef NEED_DEBUG
	struct proc_info_list *list;

	/*
	 * locate processor in the list of supported processor
	 * types.The linker builds this table for us from the 
	 * entries in arch/arm/mm/proc-*.S
	 */
	list = lookup_processor_type(read_cpuid_id());
	if(!list) {
		mm_err("CPU configuration botched(ID %p),unable"
				" to continue.\n",(void *)read_cpuid_id());
		while(1);
	}

	cpu_name = list->cpu_name;

#ifdef MULTI_CPU
	processor = *list->proc;
#endif
#ifdef MULTI_TLB
	cpu_tlb = *list->tlb;
#endif
#ifdef MULTI_CACHE
	cpu_cache = *list->cache;
#endif

	mm_debug("CPU:%s[%p]revision %p(ARM %s),cr=%p\n",
			cpu_name,read_cpuid_id(),read_cpuid_id() & 15,
			proc_arch[cpu_architecture()],cr_alignment);
	sprintf(init_utsname()->machine,"%s%c",list->arch_name,ENDIANNESS);
	sprintf(elf_platform,"%s%c",list->elf_name,ENDIANNESS);
#ifndef CONFIG_ARM_THUMB
	elf_hwcap &= ~HWCAP_THUMB;
#endif
	feat_v6_fixup();
#endif
	cacheid_init();
#ifdef NEED_DEBUG
	cpu_proc_init();
#endif
}

void __init setup_arch(char **cmdline_p)
{

	struct tag *tags = (struct tag *)&init_tags;
	struct machine_desc *mdesc;
	char *from = default_command_line;
	extern struct meminfo meminfo;
	unwind_init();

	setup_processor();

	mdesc = setup_machine(machine_arch_type);
	machine_desc = mdesc;
	machine_name = mdesc->name;
	
	if(mdesc->soft_reboot)
		reboot_setup("s");

	if(__atags_pointer)
		tags = (struct tag *)(unsigned long)__atags_pointer;
	else if(mdesc->boot_params)
		tags = (struct tag *)(unsigned long)mdesc->boot_params;
#if defined(CONFIG_DEPRECATED_PARAM_STRUCT)
	/*
	 * If we have the old style paramenters,convert them to 
	 * a tag list.
	 */
	if(tag->hdr.tag != ATAG_CORE)
		convert_to_tag_list(tags);

#endif

	if(tags->hdr.tag != ATAG_CORE)
		tags = (struct tag *)&init_tags;


	if(mdesc->fixup) 
		mdesc->fixup(mdesc,tags,&from,&meminfo);

	if(tags->hdr.tag == ATAG_CORE) {
		if(meminfo.nr_banks != 0) 
			squash_mem_tags(tags);
		save_atags(tags);
		parse_tags(tags);
	}

	init_mm.start_code  = (unsigned long)__executable_start;
	init_mm.end_code    = (unsigned long)_etext;
	init_mm.end_data    = (unsigned long)_edata;
	init_mm.brk         = (unsigned long)_end;

	/* parse_early_param needs a boot_command_line */
	strcpy(boot_command_line,from);
	/* Populate cmd_line too for later use,preserving boot_command_line */
	strcpy(cmd_line,boot_command_line);
	*cmdline_p = cmd_line;

	parse_early_param();
	
	arm_memblock_init(&meminfo);
	
	paging_init(mdesc);

	request_standard_resource(mdesc);

	reserve_crashkernel();

	if(mdesc->init_early)
		mdesc->init_early();
}

int cpu_architecture(void)
{
	int cpu_arch;

	if((read_cpuid_id() & 0x0008f000) == 0) {
		cpu_arch = CPU_ARCH_UNKNOWN;
	} else if((read_cpuid_id() & 0x0008f000) == 0x00007000) {
		cpu_arch = (read_cpuid_id() & (1 << 23)) ? 
					CPU_ARCH_ARMv4T : CPU_ARCH_ARMv3; 
	} else if((read_cpuid_id() & 0x00080000) == 0x00000000) {
		cpu_arch = (read_cpuid_id() >> 16) &7;
		if(cpu_arch)
			cpu_arch += CPU_ARCH_ARMv3;
	} else if((read_cpuid_id() & 0x000f0000) == 0x000f0000) {
		unsigned int mmfr0;

		/* 
		 * Revised CPUID format.Read the Memory Model Feature
		 * Register 0 and check for VMSAv7 or PMSAv7.
		 */
		/*
		 * asm("mrc p15,0,%0,c0,c1,4"
		 * : "=r" (mmfr0));
		 * In order to simulate the operation that read data from 
		 * C0 register of CP15,we use some way to get it.
		 */
		mmfr0 = read_cpuid_ext(CPUID_EXT_MMFR0);
		if((mmfr0 & 0x0000000f) >= 0x00000003 ||
				(mmfr0 & 0x000000f0) >= 0x00000030)
			cpu_arch = CPU_ARCH_ARMv7;
		else if((mmfr0 & 0x00000000f) == 0x00000002 ||
				(mmfr0 & 0x000000f0) == 0x00000020)
			cpu_arch = CPU_ARCH_ARMv6;
		else
			cpu_arch = CPU_ARCH_UNKNOWN;
	} else
		cpu_arch = CPU_ARCH_UNKNOWN;
	
	return cpu_arch;
}
/*
 * Pick out the memory size.We look for mem=size@start,
 * where start and size are "size[KkMm]"
 */
static int __init early_mem(char *p)
{
	static int usermem __initdata = 0;
	unsigned long size,start;
	char *endp;

	/*
	 * If the user specifies memory size,we 
	 * blow away any automatically generated
	 * size.
	 */
	if(usermem == 0) {
		usermem = 1;
		meminfo.nr_banks = 0;
	}

	start = PHYS_OFFSET;
	size  = memparse(p,&endp);
	if(*endp == '@')
		start = memparse(endp + 1 , NULL);

	arm_add_memory(start,size);

	return 0;
}
early_param("mem",early_mem);
