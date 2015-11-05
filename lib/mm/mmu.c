#include "../../include/linux/config.h"
#include "../../include/linux/types.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/highmem.h"
#include "../../include/linux/pgtable.h"
#include "../../include/linux/memblock.h"
#include "../../include/linux/kernel.h"
#include "../../include/linux/setup.h"
#include "../../include/linux/init_mm.h"
#include "../../include/linux/map.h"
#include "../../include/linux/pgalloc.h"
#include "../../include/linux/traps.h"
#include "../../include/linux/page.h"
#include "../../include/linux/memory.h"
#include <string.h>
#include <malloc.h>

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
	phys_addr_t addr = memblock_alloc(size,1);
	mem_addr_t *mem_addr = phys_to_mem(addr);
    
	memset(mem_addr,0xFF,size);
	return (void *)phys_to_virt(addr);
}
/*
 * early setup vmalloc area
 */
void early_vmalloc(void)
{
	/*
	 * For debug,we set VMALLOC area are 256M.
	 */
	unsigned long vmalloc_reserve = CONFIG_VMALLOC_RESERVE;

	if(vmalloc_reserve < SZ_16M)
	{
		mm_debug("VMALLOC too small,and limit the size 16M\n");
		vmalloc_reserve = SZ_16M;
	}
	if(vmalloc_reserve > (VMALLOC_END - (PAGE_OFFSET + SZ_32M)))
	{
		vmalloc_reserve = VMALLOC_END - (PAGE_OFFSET + SZ_32M);
		mm_debug("VMALLOC too big,and limit the size as %p\n",(void *)vmalloc_reserve);
	}
	vmalloc_min = (void *)(VMALLOC_END - vmalloc_reserve);
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
	mm_debug("[%s]\n",__FUNCTION__);
}
/*
 * sanity check meminfo
 */
static void __init sanity_check_meminfo(void)
{
	int i,j,highmem = 0;
	
	lowmem_limit = __pa(vmalloc_min - 1) + 1;
	memblock_set_current_limit(lowmem_limit);

	for(i = 0 , j = 0 ; i < meminfo.nr_banks ; i++)
	{
		struct membank *bank = &meminfo.bank[j];
		*bank = meminfo.bank[i];
		
#ifdef CONFIG_HIGHMEM
		if(__va(bank->start) > (unsigned long)vmalloc_min ||
				__va(bank->start) < PAGE_OFFSET)
			highmem = 1;
		bank->highmem = highmem;
		if(__va(bank->start) < (unsigned long)vmalloc_min &&
				bank->size > (unsigned long)vmalloc_min - __va(bank->start))
		{
			if(meminfo.nr_banks >= NR_BANKS)
				mm_err("NR_BANKS too low,ignoring high memory\n");
			else
			{
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
	 * Clear page table of Userspace.
	 */
	for( ; addr < MODULES_VADDR ; addr += PGDIR_SIZE)
		pmd_clear(pmd_off_k(addr));
	/*
	 * Clear page table of Kernle Modules.
	 */
	for( ; addr < PAGE_OFFSET ; addr += PGDIR_SIZE)
		pmd_clear(pmd_off_k(addr));
	/*
	 * Get the end of first membank.
	 */
	end = memblock.memory.regions[0].base + memblock.memory.regions[0].size;
	if(end > lowmem_limit)
		end = lowmem_limit;
	/*
	 * Clear all page table of Kernel Space except first bank.
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
	if(pmd_none(pmd))
	{
		pte_t *pte = early_alloc(2 * PTRS_PER_PTE * sizeof(pte_t));
		__pmd_populate(pmd,__pa(pte),prot);
	}
	return pte_offset_kernel(pmd,addr);
}
/*
 * pte_set
 */
void set_pte_ext(phys_addr_t addr,unsigned long pte)
{
	unsigned int *p;

	p = phys_to_mem(virt_to_phys(pte));
	*p = addr;
}
/*
 * Alloc and init pte. 
 */
void __init alloc_init_pte(pmd_t *pmd,unsigned long addr,
		unsigned long end,unsigned long pfn,unsigned long prot)
{
	pte_t *pte = early_pte_alloc(pmd,addr,prot);
	
	do {
		/* Core Deal */
		set_pte_ext(pfn_pte(pfn,0),(unsigned long)pte);
		/* --------- */
		pfn++;
	} while(pte++,addr += PAGE_SIZE,addr != end);
}
/*
 * Alloc and init section
 */
void alloc_init_section(pgd_t *pgd,unsigned long addr,
		unsigned long end,phys_addr_t phys,unsigned long prot)
{
	pmd_t *pmd = pmd_offset(pgd,addr);

	alloc_init_pte(pmd,addr,end,phys_to_pfn(phys),prot);
}
/*
 * Create mapping.
 */
void create_mapping(struct map_desc *md)
{
	unsigned long start,end,next,length;
	phys_addr_t phys;
	pgd_t *pgd;
	pgd_t *p_end;

	start = md->virtual & PAGE_MASK;
	phys  = (unsigned long)pfn_to_phys(md->pfn);
	length = PAGE_ALIGN(md->length + (md->virtual & ~PAGE_MASK));

	pgd = pgd_offset_k(start);
	p_end = pgd;
	end = start + length;
	do {
		next = pgd_addr_end(start,end);
		alloc_init_section(pgd,start,next,phys,0);
		phys += next - start;
		start = next;
	} while(pgd++,start != end);
}
/*
 * Maping lowmem.
 */
void __init map_lowmem(void)
{
	struct memblock_region *reg;

	for_each_memblock(memblock.memory,reg)
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
		md.type    = 0;

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
static void __init devicemaps_init(void)
{
	unsigned int addr;
	struct map_desc md;

	vectors_page = early_alloc(PAGE_SIZE);

	for(addr = VMALLOC_END ; addr ; addr += PGDIR_SIZE)
		pmd_clear(pmd_off_k(addr));

	/*
	 * Mapping Descrp of Vector.
	 */
	md.pfn = phys_to_pfn(virt_to_phys(vectors_page));
	md.virtual = 0xffff0000;
	md.length  = PAGE_SIZE;
	md.type    = 0;

	create_mapping(&md);

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
			PKMAP_BASE,0);
#endif
}
/*
 * Paging_init() set up the page table,initialises the zone memory
 * maps,and sets up the zero page,bad page and bad page table.
 */
void __init paging_init(void)
{
	void *zero_page;

	build_mem_type_table();
	sanity_check_meminfo();
	prepare_page_table();
	map_lowmem();
	devicemaps_init();
	kmap_init();

	top_pmd = pmd_off_k(0xFFFF0000);
	
	/*
	 * Allocate the zero page.
	 */
	zero_page = early_alloc(PAGE_SIZE);

	bootmem_init();
	empty_zero_page = virt_to_page(zero_page);
	mm_debug("empty_zero_page %p\n",(void *)empty_zero_page);
}





