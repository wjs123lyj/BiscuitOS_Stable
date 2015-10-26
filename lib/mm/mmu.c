#include "../../include/linux/types.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/highmem.h"
#include "../../include/linux/pgtable.h"
#include "../../include/linux/memblock.h"
#include "../../include/linux/kernel.h"
#include "../../include/linux/config.h"
#include "../../include/linux/setup.h"
#include <string.h>
#include <malloc.h>

void *vmalloc_min = (void *)(VMALLOC_END - SZ_128M);
phys_addr_t lowmem_limit = 0;
/*
 * Early memory allocater API in Bootmem.
 * Note,For this version can use to alloc in PAGE_SIZE.
 */
void *early_alloc(unsigned long size)
{
	phys_addr_t addr = memblock_alloc(size,1);
	mem_addr_t *mem_addr = phys_to_mem(addr);
    
	memset(mem_addr,0xFF,size);
	return mem_addr;
}
/*
 * early setup vmalloc area
 */
void early_vmalloc(void)
{
	/*
	 * For debug,we set VMALLOC area are 256M.
	 */
	unsigned int vmalloc_reserve = SZ_256M;

	if(vmalloc_reserve < SZ_16M)
	{
		mm_debug("VMALLOC too small,and limit the size 16M\n");
		vmalloc_reserve = SZ_16M;
	}
	if(vmalloc_reserve > (unsigned int)(VMALLOC_END - (PAGE_OFFSET + SZ_32M)))
	{
		mm_debug("VMALLOC too big,and limit the size\n");
		vmalloc_reserve = VMALLOC_END - (PAGE_OFFSET + SZ_32M);
	}
	vmalloc_min = VMALLOC_END - vmalloc_reserve;
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
	
	lowmem_limit = __pa(vmalloc_min);
	memblock_set_current_limit(lowmem_limit);

	for(i = 0 , j = 0 ; i < meminfo.nr_banks ; i++)
	{
		struct membank *bank = &meminfo.bank[j];
		*bank = meminfo.bank[i];
		
#ifdef CONFIG_HIGHMEM
		if(__va(bank->start) > vmalloc_min ||
				__va(bank->start) < (void *)PAGE_OFFSET)
			highmem = 1;
		bank->highmem = highmem;

		if(__va(bank->start) < vmalloc_min &&
				bank->size > vmalloc_min - __va(bank->start))
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
				bank[1].size -= (unsigned int)vmalloc_min - __va(bank->start);
				bank[1].highmem = highmem = 1;
				j++;
			}
			bank[0].size = vmalloc_min - __va(bank->start);
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
 * Paging_init() set up the page table,initialises the zone memory
 * maps,and sets up the zero page,bad page and bad page table.
 */
void __init paging_init(void)
{
	void *zero_page;

	build_mem_type_table();
	sanity_check_meminfo();

	bootmem_init();
}





