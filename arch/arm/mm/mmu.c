#include "linux/kernel.h"
#include "linux/page.h"
#include "linux/highmem.h"
#include "linux/map.h"
#include "linux/pgtable.h"
#include "linux/memblock.h"
#include "linux/setup.h"
#include "linux/mm.h"
#include "linux/map.h"
#include "linux/pgtable-hwdef.h"
#include "linux/page.h"
#include "asm/system.h"
#include "linux/io.h"
#include "linux/vmalloc.h"
#include "linux/domain.h"
#include "linux/memory.h"
#include "linux/cputype.h"
#include "linux/tlbflush.h"
#include "linux/init.h"
#include "linux/cachetype.h"
#include "asm/smp_plat.h"
#include "asm/arch.h"
#include "asm/head.h"
#include "linux/cachetype.h"
#include "linux/pgalloc.h"
#include "linux/memory.h"
#include "linux/pgalloc.h"
#include "linux/cacheflush.h"


extern void *vectors_page;
extern struct meminfo meminfo;
extern pgprot_t protection_map[16];
extern unsigned long long memparse(const char *ptr,char **retptr);
long __init_memblock memblock_reserve(phys_addr_t base,phys_addr_t size);

#define CPOLICY_UNCACHED        0
#define CPOLICY_BUFFERED        1
#define CPOLICY_WRITETHROUGH    2
#define CPOLICY_WRITEBACK       3
#define CPOLICY_WRITEALLOC      4

#define PROT_PTE_DEVICE  L_PTE_PRESENT | L_PTE_YOUNG | L_PTE_DIRTY | L_PTE_XN
#define PROT_SECT_DEVICE PMD_TYPE_SECT | PMD_SECT_AP_WRITE

struct cachepolicy {
	const char policy[16];
	unsigned int cr_mask;
	unsigned int pmd;
	pteval_t pte;
};

static unsigned long cr_no_alignment;
static unsigned long cr_alignment;
static struct cachepolicy cache_policies[] __initdata = {
	{
		.policy     = "uncached",
		.cr_mask    = CR_W | CR_C,
		.pmd        = PMD_SECT_UNCACHED,
		.pte        = L_PTE_MT_UNCACHED,
	},
	{
		.policy     = "buffered",
		.cr_mask    = CR_C,
		.pmd        = PMD_SECT_BUFFERED,
		.pte        = L_PTE_MT_BUFFERABLE,
	},
	{
		.policy     = "writethrough",
		.cr_mask    = 0,
		.pmd        = PMD_SECT_WT,
		.pte        = L_PTE_MT_WRITETHROUGH,
	},
	{
		.policy     = "writeback",
		.cr_mask    = 0,
		.pmd        = PMD_SECT_WB,
		.pte        = L_PTE_MT_WRITEBACK,
	},
	{
		.policy     = "writealloc",
		.cr_mask    = 0,
		.pmd        = PMD_SECT_WB,
		.pte        = L_PTE_MT_WRITEBACK,
	}
};

static struct mem_type mem_types[] = {
	[MT_DEVICE] = { /* Strongly ordered/ARMv6 shared device */
		.prot_pte   = PROT_PTE_DEVICE | L_PTE_MT_DEV_SHARED |
			          L_PTE_SHARED,
		.prot_l1    = PMD_TYPE_TABLE,
		.prot_sect  = PROT_SECT_DEVICE | PMD_SECT_S,
		.domain     = DOMAIN_IO,
	},
	[MT_DEVICE_NONSHARED] = { /* ARMv6 non-shared device */
		.prot_pte   = PROT_PTE_DEVICE | L_PTE_MT_DEV_NONSHARED,
		.prot_l1    = PMD_TYPE_TABLE,
		.prot_sect  = PROT_SECT_DEVICE,
		.domain     = DOMAIN_IO,
	},
	[MT_DEVICE_CACHED] = { /* ioremap_cached */
		.prot_pte   = PROT_PTE_DEVICE | L_PTE_MT_DEV_CACHED,
		.prot_l1    = PMD_TYPE_TABLE,
		.prot_sect  = PROT_SECT_DEVICE | PMD_SECT_WB,
		.domain     = DOMAIN_IO,
	},
	[MT_DEVICE_WC] = { /* ioremap_wc */
		.prot_pte   = PROT_PTE_DEVICE | L_PTE_MT_DEV_WC,
		.prot_l1    = PMD_TYPE_TABLE,
		.prot_sect  = PMD_TYPE_SECT | PMD_SECT_XN,
		.domain     = DOMAIN_IO,
	},
	[MT_UNCACHED] = {
		.prot_pte   = PROT_PTE_DEVICE,
		.prot_l1    = PMD_TYPE_TABLE,
		.prot_sect  = PMD_TYPE_SECT | PMD_SECT_XN,
		.domain     = DOMAIN_IO,
	},
	[MT_CACHECLEAN] = {
		.prot_sect  = PMD_TYPE_SECT | PMD_SECT_XN,
		.domain     = DOMAIN_KERNEL,
	},
	[MT_MINICLEAN]  = {
		.prot_sect  = PMD_TYPE_SECT | PMD_SECT_XN | PMD_SECT_MINICACHE,
		.domain     = DOMAIN_KERNEL,
	},
	[MT_LOW_VECTORS] = {
		.prot_pte   = L_PTE_PRESENT | L_PTE_YOUNG | L_PTE_DIRTY |
			          L_PTE_RDONLY,
		.prot_l1    = PMD_TYPE_TABLE,
		.domain     = DOMAIN_USER,
	},
	[MT_HIGH_VECTORS] = {
		.prot_pte   = L_PTE_PRESENT | L_PTE_YOUNG | L_PTE_DIRTY |
			          L_PTE_USER    | L_PTE_RDONLY,
		.prot_l1    = PMD_TYPE_TABLE,
		.domain     = DOMAIN_USER,
	},
	[MT_MEMORY] = {
		.prot_pte   = L_PTE_PRESENT | L_PTE_YOUNG | L_PTE_DIRTY,
		.prot_l1    = PMD_TYPE_TABLE,
		.prot_sect  = PMD_TYPE_SECT | PMD_SECT_AP_WRITE,
		.domain     = DOMAIN_KERNEL,
	},
	[MT_ROM] = {
		.prot_pte   = PMD_TYPE_SECT,
		.domain     = DOMAIN_KERNEL,
	},
	[MT_MEMORY_NONCACHED] = {
		.prot_pte   = L_PTE_PRESENT | L_PTE_YOUNG | L_PTE_DIRTY |
			          L_PTE_MT_BUFFERABLE,
		.prot_l1    = PMD_TYPE_TABLE,
		.prot_sect  = PMD_TYPE_SECT | PMD_SECT_AP_WRITE,
		.domain     = DOMAIN_KERNEL,
	},
	[MT_MEMORY_DTCM] = {
		.prot_pte   = L_PTE_PRESENT | L_PTE_YOUNG | L_PTE_DIRTY |
			          L_PTE_XN,
		.prot_l1    = PMD_TYPE_TABLE,
		.prot_sect  = PMD_TYPE_SECT | PMD_SECT_XN,
		.domain     = DOMAIN_KERNEL,
	},
	[MT_MEMORY_ITCM] = {
		.prot_pte   = L_PTE_PRESENT | L_PTE_YOUNG | L_PTE_DIRTY,
		.prot_l1    = PMD_TYPE_TABLE,
		.domain     = DOMAIN_KERNEL,
	},
};

static unsigned int ecc_mask __initdata = 0;
pgprot_t pgprot_user;
pgprot_t pgprot_kernel;

#define CPOLICY_UNCACHED       0
#define CPOLICY_BUFFERED       1
#define CPOLICY_WRITETHROUGH   2
#define CPOLICY_WRITEBACK      3
#define CPOLICY_WRITEALLOC     4

static unsigned int cachepolicy __initdata = CPOLICY_WRITEBACK;

void *vmalloc_min = (void *)(VMALLOC_END - SZ_128M);
phys_addr_t lowmem_limit = 0;
/*
 * The pmd table for the upper-most set of pages.
 */
pmd_t *top_pmd;
/*
 * Empty_zero_page is a special page that is used for
 * zero-initlized and COW.
 */
struct page *empty_zero_page;
/*
 * Early memory allocater API in Bootmem.
 * Note,For this version can use to alloc in PAGE_SIZE.
 */
void *early_alloc(unsigned long size)
{
	phys_addr_t addr = memblock_alloc(size,size);
	mem_addr_t *mem_addr = phys_to_mem(addr);
   
	/*
	 * In order to operation memory directly,we use Virtual Memory address
	 * to simulate the physical address.
	 */
	memset(mem_addr,0,size);
	/*
	 * Note,for ignoring the Virtual Memory address,we return an virtual address
	 * to replace the virtual memory address.
	 */
	return (void *)phys_to_virt(addr);
}
/*
 * Reserve global pagetable.
 */
void arm_mm_memblock_reserve(void)
{
	memblock_reserve(__pa(swapper_pg_dir),PTRS_PER_PGD * sizeof(pgd_t));
}
/*
 * Adjust the PMD section entries accoring to the CPU in use.
 */
static void __init build_mem_type_table(void)
{
	struct cachepolicy *cp;
	unsigned int cr = get_cr();
	unsigned int user_pgprot,kern_pgprot,vecs_pgprot;
	int cpu_arch = cpu_architecture();
	int i;

	if(cpu_arch < CPU_ARCH_ARMv6) {
#if defined(CONFIG_CPU_DCACHE_DISABLE)
		if(cachepolicy > CPOLICY_BUFFERED)
			cachepolicy = CPOLICY_BUFFERED;
#elif defined(CONFIG_CPU_DCACHE_WRITETHROUGH)
		if(cachepolicy > CPOLICY_WRITETHROUGH)
			cachepolicy = CPOLICY_WRITETHROUGH;
#endif
	}
	if(cpu_arch < CPU_ARCH_ARMv5) {
		if(cachepolicy >= CPOLICY_WRITEALLOC)
			cachepolicy = CPOLICY_WRITEBACK;
		ecc_mask = 0;
	}
	if(is_smp())
		cachepolicy = CPOLICY_WRITEALLOC;

	/*
	 * Strip out feature not present on earlier architetures.
	 * Pre-ARMv5 CPUs don't have TEX bits.Pre-ARMv6 CPUs or those
	 * without extended page table don't have the 'Shared' bit.
	 */
	if(cpu_arch < CPU_ARCH_ARMv5)
		for(i = 0 ; i < ARRAY_SIZE(mem_types) ; i++)
			mem_types[i].prot_sect &= ~PMD_SECT_TEX(7);
	if((cpu_arch < CPU_ARCH_ARMv6 || !(cr & CR_XP)) && !cpu_is_xsc3())
		for(i = 0 ; i < ARRAY_SIZE(mem_types) ; i++)
			mem_types[i].prot_sect &= ~PMD_SECT_S;

	/*
	 * ARMv5 and lower,bit 4 must be set for page tables(was:cache
	 * "update-able on write" bit on ARM6 10).However,Xscale and
	 * Xscale3 require this bit to be cleared.
	 */
	if(cpu_is_xscale() || cpu_is_xsc3()) {
		for(i = 0 ; i < ARRAY_SIZE(mem_types) ; i++) {
			mem_types[i].prot_sect &= ~PMD_BIT4;
			mem_types[i].prot_l1   &= ~PMD_BIT4;
		}
	} else if(cpu_arch < CPU_ARCH_ARMv6) {
		for(i = 0 ; i < ARRAY_SIZE(mem_types) ; i++) {
			if(mem_types[i].prot_l1)
				mem_types[i].prot_l1 |= PMD_BIT4;
			if(mem_types[i].prot_sect)
				mem_types[i].prot_sect |= PMD_BIT4;
		}
	}

	/*
	 * Mark the device areas according to the CPU/architecture.
	 */
	if(cpu_is_xsc3() || (cpu_arch >= CPU_ARCH_ARMv6 && (cr & CR_XP))) {
		if(!cpu_is_xsc3()) {
			/*
			 * Mark device regions on ARMv6+ as execute-never
			 * to prevent speculative instruction fetches.
			 */
			mem_types[MT_DEVICE].prot_sect |= PMD_SECT_XN;
			mem_types[MT_DEVICE_NONSHARED].prot_sect |= PMD_SECT_XN;
			mem_types[MT_DEVICE_CACHED].prot_sect |= PMD_SECT_XN;
			mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_XN;
		}
		if(cpu_arch > CPU_ARCH_ARMv7 && (cr & CR_TRE)) {
			/*
			 * For ARMv7 with TEX remapping,
			 * -shared device is SXCB   = 1100
			 * -noshared device is SXCB = 0100
			 * -write combine device mem is SXCB = 0001
			 * (Uncached Normal memory)
			 */
			mem_types[MT_DEVICE].prot_sect |= PMD_SECT_TEX(1);
			mem_types[MT_DEVICE_NONSHARED].prot_sect |= PMD_SECT_TEX(1);
			mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_BUFFERABLE;
		} else if(cpu_is_xsc3()) {
			/*
			 * For Xscale3,
			 * - shared device is TEXCB     = 00101
			 * - nonshared device is TEXCB  = 01000
			 * - write combine device mem is TEXCB = 00100
			 * (Inner/Outer Uncacheable in xsc3 parlance)
			 */
			mem_types[MT_DEVICE].prot_sect |= 
				PMD_SECT_TEX(1) | PMD_SECT_BUFFERED;
			mem_types[MT_DEVICE_NONSHARED].prot_sect |= PMD_SECT_TEX(2);
			mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_TEX(1);
		} else {
			/*
			 * For ARMv6 and ARMv7 without TEX remapping,
			 * - shared device is TEXCB = 00001
			 * - nonshared device is TEXCB = 01000
			 * - write combine device mem is TEXCB = 00100
			 * (Uncached Normal in ARMv6 parlance)
			 */
			mem_types[MT_DEVICE].prot_sect |= PMD_SECT_BUFFERED;
			mem_types[MT_DEVICE_NONSHARED].prot_sect |= PMD_SECT_TEX(2);
			mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_TEX(1);
		}
	} else {
		/*
		 * On others,write combining is "Uncached/Buffered"
		 */
		mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_BUFFERABLE;
	}

	/*
	 * Now deal with memory-type mappings
	 */
	cp = &cache_policies[cachepolicy];
	vecs_pgprot = kern_pgprot = user_pgprot = cp->pte;

	/*
	 * Only use write-through for non-SMP system.
	 */
	if(!is_smp() && cpu_arch >= CPU_ARCH_ARMv5 &&
			cachepolicy > CPOLICY_WRITETHROUGH) 
		vecs_pgprot = cache_policies[CPOLICY_WRITETHROUGH].pte;
	/*
	 * Enable CPU-specific coherency if supported.
	 * (Only available on XSC3 at the moment.)
	 */
	if(arch_is_coherent() && cpu_is_xsc3()) {
		mem_types[MT_MEMORY].prot_sect |= PMD_SECT_S;
		mem_types[MT_MEMORY].prot_pte  |= L_PTE_SHARED;
		mem_types[MT_MEMORY_NONCACHED].prot_sect |= PMD_SECT_S;
		mem_types[MT_MEMORY_NONCACHED].prot_pte  |= L_PTE_SHARED;
	}
	/*
	 * ARMv6 and above have extended page tables.
	 */
	if(cpu_arch >= CPU_ARCH_ARMv6 && (cr & CR_XP)) {
		/*
		 * Mark cache clean area and XIP ROM read only
		 * from SVC mode and no access from userspace.
		 */
		mem_types[MT_ROM].prot_sect |= PMD_SECT_APX | PMD_SECT_AP_WRITE;
		mem_types[MT_MINICLEAN].prot_sect |= PMD_SECT_APX | PMD_SECT_AP_WRITE;
		mem_types[MT_CACHECLEAN].prot_sect |= PMD_SECT_APX | PMD_SECT_AP_WRITE;

		if(is_smp()) {
			/*
			 * Mark memory with the "shared" attribute
			 * for SMP systems.
			 */
			user_pgprot |= L_PTE_SHARED;
			kern_pgprot |= L_PTE_SHARED;
			vecs_pgprot |= L_PTE_SHARED;
			mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_S;
			mem_types[MT_DEVICE_WC].prot_pte  |= L_PTE_SHARED;
			mem_types[MT_DEVICE_CACHED].prot_sect |= PMD_SECT_S;
			mem_types[MT_DEVICE_CACHED].prot_pte  |= L_PTE_SHARED;
			mem_types[MT_MEMORY].prot_sect |= PMD_SECT_S;
			mem_types[MT_MEMORY].prot_pte  |= L_PTE_SHARED;
			mem_types[MT_MEMORY_NONCACHED].prot_sect |= PMD_SECT_S;
			mem_types[MT_MEMORY_NONCACHED].prot_pte  |= L_PTE_SHARED;
		}
	}

	/*
	 * Non-cacheable Normal - intended for memory areas that must
	 * not cause dirty cache line writebacks when used.
	 */
	if(cpu_arch >= CPU_ARCH_ARMv6) {
		if(cpu_arch >= CPU_ARCH_ARMv7 && (cr & CR_TRE)) {
			/* Non-cacheable Normal is VCB = 001 */
			mem_types[MT_MEMORY_NONCACHED].prot_sect |=
				PMD_SECT_BUFFERED;
		} else {
			/* For both ARMv6 and non-TEX-remapping ARMv7 */
			mem_types[MT_MEMORY_NONCACHED].prot_sect |=
				PMD_SECT_TEX(1);
		}
	} else {
		mem_types[MT_MEMORY_NONCACHED].prot_sect |= PMD_SECT_BUFFERABLE;
	}

	for(i = 0 ; i < 16 ; i++) {
		unsigned long v = (unsigned long)pgprot_val(protection_map[i]);
		protection_map[i] = __pgprot(v | user_pgprot);
	}

	mem_types[MT_LOW_VECTORS].prot_pte  |= vecs_pgprot;
	mem_types[MT_HIGH_VECTORS].prot_pte |= vecs_pgprot;

	pgprot_user    = __pgprot(L_PTE_PRESENT | L_PTE_YOUNG | user_pgprot);
	pgprot_kernel  = __pgprot(L_PTE_PRESENT | L_PTE_YOUNG |
							  L_PTE_DIRTY | kern_pgprot);

	mem_types[MT_LOW_VECTORS].prot_l1   |= ecc_mask;
	mem_types[MT_HIGH_VECTORS].prot_l1  |= ecc_mask;
	mem_types[MT_MEMORY].prot_sect |= ecc_mask | cp->pmd;
	mem_types[MT_MEMORY].prot_pte  |= kern_pgprot;
	mem_types[MT_MEMORY_NONCACHED].prot_sect |= ecc_mask;
	mem_types[MT_ROM].prot_sect   |= cp->pmd;

	switch(cp->pmd)
	{
		case PMD_SECT_WT:
			mem_types[MT_CACHECLEAN].prot_sect |= PMD_SECT_WT;
			break;
		case PMD_SECT_WB:
		case PMD_SECT_WBWA:
			mem_types[MT_CACHECLEAN].prot_sect |= PMD_SECT_WB;
			break;
	}
	mm_debug("Memory policy:ECC %sabled,Data cache %s\n",
			ecc_mask ? "en" : "dis",cp->policy);

	for(i = 0 ; i < ARRAY_SIZE(mem_types) ; i++)
	{
		struct mem_type *t = &mem_types[i];

		if(t->prot_l1)
			t->prot_l1 |= PMD_DOMAIN(t->domain);
		if(t->prot_sect)
			t->prot_sect |= PMD_DOMAIN(t->domain);
	}
}
/*
 * sanity check meminfo
 */
static void __init sanity_check_meminfo(void)
{
	int i,j,highmem = 0;

	lowmem_limit = __pa(vmalloc_min - 1) + 1;
	memblock_set_current_limit(lowmem_limit);

	for(i = 0 , j = 0 ; i < meminfo.nr_banks ; i++) {
		struct membank *bank = &meminfo.bank[j];
		*bank = meminfo.bank[i];
		
#ifdef CONFIG_HIGHMEM
		if(__va(bank->start) > (unsigned long)vmalloc_min ||
				__va(bank->start) < PAGE_OFFSET)
			highmem = 1;
		bank->highmem = highmem;
		if(__va(bank->start) < (unsigned long)vmalloc_min &&
				bank->size > (unsigned long)vmalloc_min - __va(bank->start)) {
			if(meminfo.nr_banks >= NR_BANKS)
				mm_err("NR_BANKS too low,ignoring high memory\n");
			else {
				memmove(bank + 1,bank,
						(meminfo.nr_banks - i) * sizeof(struct membank));
				meminfo.nr_banks++;
				i++;
				bank[1].start = __pa(vmalloc_min -1) + 1;
				bank[1].size -= (unsigned long)vmalloc_min - __va(bank->start);
				bank[1].highmem = highmem = 1;
				j++;
			}
			bank[0].size = (unsigned long)vmalloc_min - __va(bank->start);
		}
#else
		bank->highmem = highmem;
		/*
		 * Check whether this memory bank would entrirely overlap
		 * the vmalloc area.
		 */
		if(__va(bank->start) > vmalloc_min ||
				__va(bank->start) < (void *)PAGE_OFFSET)
		{
			mm_err("Ignoring RAM at %08x - %08x\n",bank->start,
					bank->start + bank->size);
			continue;
		}
		/*
		 * Check whether this memory bank would partially overlap
		 * the vmalloc area.
		 */
		if(__va(bank->start + bank->size) > vmalloc_min ||
				__va(bank->start + bank->size) < __va(bank->start))
		{
			unsigned int newsize = vmalloc_min - __va(bank->start);
			mm_debug("Truncating RAM[%08x - %08x]Ignoring[%08x - %08x]\n",
					bank->start,bank->start + bank->size,
					bank->start + newsize,bank->size + bank->start);
			bank->size = newsize;
		}
#endif
		j++;
	}
#ifdef CONFIG_HIGHMEM
	if(highmem) {
		const char *reason = NULL;
		if(cache_is_vipt_aliasing()) {
			/*
			 * Interactions between kmap and other mappings
			 * make highmem support with aliasing VIPT caches
			 * rather difficult.
			 */
			reason = "with VIPT aliasing cache";
		} else if(is_smp() && tlb_ops_need_broadcast()) {
			/*
			 * kmap_high needs to occasionally flush TLB entries,
			 * however,if the TLB entries need to be broadcast
			 * we may deadlock:
			 * kmap_high(irq off)->flush_all_zero_pkmaps->
			 * flush_tlb_kernel_range->smp_call_function_many
			 * (must not be called with irqs off)
			 */
			reason = "without hardware TLB ops broadcasting";
		}
		if(reason) {
			mm_debug("HIGHMEM is not supported %s,ignoring high memory\n",
					reason);
			while(j > 0 && meminfo.bank[j - 1].highmem)
				j--;
		}
	}
#endif
	meminfo.nr_banks = j;
}
/*
 * Prepare page talbe 
 */
static inline void prepare_page_table(void)
{
	unsigned long addr = 0;
	phys_addr_t end;

	/*
	 * Clear out all the mappings below the kernel image.
	 */
	for( ; addr < MODULES_VADDR ; addr += PGDIR_SIZE) 
		pmd_clear(pmd_off_k(addr));
		
	/*
	 * Clear page table of Kernle Modules.
	 */
	for( ; addr < PAGE_OFFSET ; addr += PGDIR_SIZE)
		pmd_clear(pmd_off_k(addr));
	
	/*
	 * Find the end of the first block of lowmem. 
	 */
	end = memblock.memory.regions[0].base + memblock.memory.regions[0].size;
	if(end > lowmem_limit)
		end = lowmem_limit;

	/*
	 * Clear out all the kernel space mappings,except for the first
	 * memory bank,up to the end of the vmalloc region.
	 */
	for(addr = (unsigned long)phys_to_virt(end) ; addr < VMALLOC_END ; 
			addr += PGDIR_SIZE)
		pmd_clear(pmd_off_k(addr));
}
/*
 * Early pte alloc
 */
static pte_t * __init early_pte_alloc(pmd_t *pmd,unsigned long addr,
		unsigned long prot)
{
	if(pmd_none(pmd)) {
		pte_t *pte = early_alloc(2 * PTRS_PER_PTE * sizeof(pte_t));
		__pmd_populate(pmd,__pa(pte),prot);
	}
	BUG_ON(pmd_bad(pmd));
	
	return pte_offset_kernel(pmd,addr);
}
/*
 * Alloc and init pte. 
 */
void __init alloc_init_pte(pmd_t *pmd,unsigned long addr,
		unsigned long end,unsigned long pfn,
		const struct mem_type *type)
{
	pte_t *pte = early_pte_alloc(pmd,addr,type->prot_l1);
	do {
	//	set_pte_ext(pte,pfn_pte(pfn,__pgprot(type->prot_pte)),0);
		pfn++;
	} while(pte++,addr += PAGE_SIZE,addr != end);
}
/*
 * Alloc and init section
 */
void alloc_init_section(pgd_t *pgd,unsigned long addr,
		unsigned long end,phys_addr_t phys,
		const struct mem_type *type)
{
	pmd_t *pmd = pmd_offset(pgd,addr);

	/*
	 * Try a section mapping - end,addr and phys must all be aligned
	 * to a section boundary.Note that PMDs refer to the individual
	 * L1 entries,whereas PGDs refer to a group of L1 entries making
	 * up one logical pointer to an L2 table.
	 */
	if(((addr | end | phys) & ~SECTION_MASK) == 0) {
		pmd_t *p = pmd;

		if(addr & SECTION_SIZE)
			pmd++;

		do {
			/*
			 * In order to simalute pmd table,we use virtual address 
			 * to store pmd_val.
			 */
			pmd_t *pmd_p = 
				(pmd_t *)(unsigned long)phys_to_mem(__pa(pmd));
			*pmd_p = __pmd(phys | type->prot_sect);
			phys += SECTION_SIZE;
		} while(pmd++,addr += SECTION_SIZE , addr != end);

		flush_pmd_entry(p);
	} else {
		/*
		 * No need to loop:pte's aren't interested in the 
		 * individual L1 entries.
		 */
		alloc_init_pte(pmd,addr,end,__phys_to_pfn(phys),type);
	}
}
static void __init create_36bit_mapping(struct map_desc *md,
		const struct mem_type *type)
{
	unsigned long addr,length,end;
	phys_addr_t phys;
	pgd_t *pgd;

	addr = md->virtual;
	phys = (unsigned long)__pfn_to_phys(md->pfn);
	length = PAGE_ALIGN(md->length);

	if(!(cpu_architecture() >= CPU_ARCH_ARMv6 || cpu_is_xsc3())) {
		mm_err("MM:CPU doesn't support supersection "
				"mapping for %p at %p\n",
				(void *)__pfn_to_phys((u64)md->pfn),(void *)addr);
		return;
	}
	/*
	 * ARMv6 supersection are only defined to work with domain 0.
	 * Since domain assignments can in fact be arbitrary,the
	 * 'domain == 0' check below is required to insure that ARMv6
	 * supersections are only allocated for domain 0 regardless
	 * of the actual domain assignments in use.
	 */
	if(type->domain) {
		mm_err("MM:invalid domain in supersection "
				"mapping for %p at %p\n",
				(void *)__pfn_to_phys((u64)md->pfn),(void *)addr);
		return;
	}
	
	if((addr | length | __pfn_to_phys(md->pfn)) & ~SUPERSECTION_MASK) {
		mm_err("MM:cannot create mapping for "
				"%p at %p invalid alignment\n",
				(void *)__pfn_to_phys((u64)md->pfn),(void *)addr);
		return;
	}

	/*
	 * Shift bits[35:32] of address into bits[23:20] of PMD.
	 */
	phys |= (((md->pfn >> (32 - PAGE_SHIFT)) & 0xF) << 20);

	pgd = pgd_offset_k(addr);
	end = addr + length;

	do {
		pmd_t *pmd = pmd_offset(pgd,addr);
		int i;
	
		for(i = 0 ; i < 16 ; i++) 
			//*pmd++ = __pmd(phys | type->prot_sect | PMD_SECT_SUPER);
				;

		addr += SUPERSECTION_SIZE;
		phys += SUPERSECTION_SIZE;
		pgd  += SUPERSECTION_SIZE >> PGDIR_SHIFT;
	} while(addr != end);
}
#define vectors_base()  (vectors_high() ? 0xffff0000 : 0)
/*
 * Create the page directory entries and any necessary page
 * tables for the mapping specified by 'md'.We are able to cope
 * here with varying sizes and address offsets,and we take full 
 * advantage of sections and supersections.
 */
static void __init create_mapping(struct map_desc *md)
{
	unsigned long phys,addr,length,end;
	const struct mem_type *type;
	pgd_t *pgd;

	if(md->virtual != vectors_base() && md->virtual < TASK_SIZE) {
		mm_warn("BUG:not creating mapping for %p at %p in user region\n",
				(void *)__pfn_to_phys(md->pfn),(void *)md->virtual);
		return;
	}

	if((md->type == MT_DEVICE || md->type == MT_ROM) &&
			md->virtual >= PAGE_OFFSET && md->virtual < VMALLOC_END) {
		mm_warn("BUG:mapping for %p at %p overlaps vmalloc space\n",
				(void *)__pfn_to_phys(md->pfn),(void *)md->virtual);
	}

	type = &mem_types[md->type];

	/*
	 * Catch 36-bit addresses
	 */
	if(md->pfn >= 0x100000) {
		create_36bit_mapping(md,type);
		return;
	}

	addr = md->virtual & PAGE_MASK;
	phys = (unsigned long)__pfn_to_phys(md->pfn);
	length = PAGE_ALIGN(md->length + (md->virtual & ~PAGE_MASK));

	if(type->prot_l1 == 0 && ((addr | phys | length) & ~SECTION_MASK)) {
		mm_warn("BUG:map for %p at %p can't be mapped using pages,ignoring\n",
				(void *)__pfn_to_phys(md->pfn),(void *)addr);
		return;
	}
	
	pgd = pgd_offset_k(addr);
	end = addr + length;
	do {
		unsigned long next = pgd_addr_end(addr,end);

		alloc_init_section(pgd,addr,next,phys,type);

		phys += next - addr;
		addr = next;
	} while(pgd++,addr != end);
}
/*
 * Maping lowmem.
 */
void __init map_lowmem(void)
{
	struct memblock_region *reg;

	/* Map all the lowmem memory banks. */
	for_each_memblock(memory,reg)
	{
		phys_addr_t start = reg->base;
		phys_addr_t end   = start + reg->size;
		struct map_desc md;

		if(end > lowmem_limit)
			end = lowmem_limit;
		if(start >= end)
			break;

		md.pfn = phys_to_pfn(start);
		md.virtual = phys_to_virt(start);
		md.length  = end - start;
		md.type    = MT_MEMORY;
		create_mapping(&md);
	}
}
/*
 * Set up device the mappings.Since we clear out the page tables for all
 * mapping above VMALLOC_END,we will remove any debug device mappings.
 * This means you have to be careful how you debug this function.or any
 * called function.This means you can't use any function or debugging
 * method which may touch any device,otherwise the kernel _will_crash.
 */
static void __init devicemaps_init(struct machine_desc *mdesc)
{
	struct map_desc map;
	unsigned int addr;

	/*
	 * Allocate the vector page early.
	 */
	vectors_page = early_alloc(PAGE_SIZE);

	for(addr = VMALLOC_END ; addr ; addr += PGDIR_SIZE)
		pmd_clear(pmd_off_k(addr));
		

	/*
	 * Mapping Descrp of Vector.
	 */
#ifdef FLUSH_BASE
	map.pfn = __phys_to_pfn(FLUSH_BASE_PHYS);
	map.virtual = FLUSH_BASE;
	map.length = SZ_1M;
	map.type = MT_CACHECLEAN;
	create_mapping(&map);
#endif
#ifdef FLUSH_BASE_MINICACHE
	map.pfn = __phys_to_pfn(FLUSH_BASE_PHYS + SZ_1M);
	map.virtual = FLUSH_BASE_MINICACHE;
	map.length = SZ_1M;
	map.type = MT_MINICLEAN;
	create_mapping(&map);
#endif

	/*
	 * Create a mapping for the machine vectors at the high-vectors
	 * location (0xffff0000).If we aren't using high-vectors,also
	 * create a mapping at the low-vectors virtual address.
	 */
	map.pfn = __phys_to_pfn(virt_to_phys(vectors_page));
	map.virtual = 0xffff0000;
	map.length  = PAGE_SIZE;
	map.type    = MT_HIGH_VECTORS;
	create_mapping(&map);

	if(!vectors_high()) {
		map.virtual = 0;
		map.type = MT_LOW_VECTORS;
		create_mapping(&map);
	}

	/*
	 * Ask the machine support to map in the statically mapped devices.
	 */
	if(mdesc->map_io)
		mdesc->map_io();

	/*
	 * Finally flush the caches and tlb to ensure that we're in a
	 * consistent state wrt the writebuffer.This also ensures that
	 * any write-allocated cache lines in the vector page are written
	 * back.After this point,we can start to touch devices again.
	 */
	local_flush_tlb_all();
	flush_cache_all();
}
/*
 * Init kmap.
 */
static void __init kmap_init(void)
{
#ifdef CONFIG_HIGHMEM
	pkmap_page_table = early_pte_alloc(pmd_off_k(PKMAP_BASE),
			PKMAP_BASE,_PAGE_KERNEL_TABLE);
#endif
}
/*
 * Paging_init() set up the page table,initialises the zone memory
 * maps,and sets up the zero page,bad page and bad page table.
 */
void __init paging_init(struct machine_desc *mdesc)
{
	void *zero_page;

	build_mem_type_table();
	sanity_check_meminfo();
	prepare_page_table();
	map_lowmem();
	devicemaps_init(mdesc);
	kmap_init();

	top_pmd = pmd_off_k(0xffff0000);
	
	/* Allocate the zero page. */
	zero_page = early_alloc(PAGE_SIZE);
	
	bootmem_init();

	empty_zero_page = virt_to_page(zero_page);
	__flush_dcache_page(NULL,empty_zero_page);
}

/*
 * These are useful for identifying cache coherency
 * problems by allowing the cache or the cache and 
 * writebuffer to be turned off.
 */
static int __init early_cachepolicy(char *p)
{
	int i;
	u32 cr_alignment;
	u32 cr_no_alignment;

	for(i = 0 ; i < ARRAY_SIZE(cache_policies) ; i++) {
		int len = strlen(cache_policies[i].policy);

		if(memcmp(p,cache_policies[i].policy,len) == 0) {
			cachepolicy = i;
			cr_alignment &= ~cache_policies[i].cr_mask;
			cr_no_alignment &= ~cache_policies[i].cr_mask;
			break;
		}
	}
	if(i == ARRAY_SIZE(cache_policies))
		mm_debug("ERROR:unknow or unsupported cache policy\n");

	/*
	 * This restriction is partly to do with the way we boot;it is
	 * unpredictable to have memory mapped using two different sets of
	 * memory attributes(shared,type,and cache attribute).we can not
	 * change these attributes once the initial assembly has setup the 
	 * page tables.
	 */
	if(cpu_architecture() >= CPU_ARCH_ARMv6) {
		mm_debug("Only cachepolicy=writeback supported on ARMv6 and later\n");
		cachepolicy = CPOLICY_WRITEBACK;
	}
	flush_cache_all();
	set_cr2(cr_alignment);
	return 0;
}
early_param("cachepolicy",early_cachepolicy);

static int __init early_nocache(char *__unused) 
{
	char *p = "buffered";
	
	mm_warn("nocache is deprecated;use cachepolicy=%s\n",p);
	early_cachepolicy(p);
	return 0;
}
early_param("nocache",early_nocache);

static int __init early_nowrite(char *__unused)
{
	char *p = "uncached";

	mm_warn("nowb is deprecated;use cachepolicy= %s\n",p);
	early_cachepolicy(p);
	return 0;
}
early_param("nowb",early_nowrite);

static int __init early_ecc(char *p)
{
	if(memcmp(p,"on",2) == 0)
		ecc_mask = PMD_PROTECTION;
	else if(memcmp(p,"off",3) == 0)
		ecc_mask = 0;
	return 0;
}

static int __init noalign_setup(char *__unused)
{
	cr_alignment &= ~CR_A;
	cr_no_alignment &= ~CR_A;
	set_cr1(cr_alignment);
	return 1;
}
__setup("noalign",noalign_setup);

/*
 * vmalloc = size forces the vmalloc area to be exactly 'size'
 * bytes.This can be used to increase the vmalloc area
 * the default is 128M.
 */
static int __init early_vmalloc(char *arg)
{
	unsigned long vmalloc_reserve = memparse(arg,NULL);

	if(vmalloc_reserve < SZ_16M) {
		vmalloc_reserve = SZ_16M;
		mm_warn("vmalloc area too small,limiting to %pMB\n",
				(void *)(vmalloc_reserve >> 20));
	}

	if(vmalloc_reserve > VMALLOC_END - (PAGE_OFFSET + SZ_32M)) {
		vmalloc_reserve = VMALLOC_END - (PAGE_OFFSET + SZ_32M);
		mm_warn("Vmalloc area is too big,limit to %pMB\n",
				(void *)(vmalloc_reserve >> 20));
	}

	vmalloc_min = (void *)(VMALLOC_END - vmalloc_reserve);
	return 0;
}
early_param("vmalloc",early_vmalloc);
