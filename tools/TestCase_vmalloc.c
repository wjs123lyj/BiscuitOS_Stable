/*
 * This file use to debug Vmalloc Allocator.
 */
#include "linux/kernel.h"
#include "linux/debug.h"
#include "linux/vmalloc.h"
#include "linux/gfp.h"
#include "linux/rbtree.h"
#include "linux/mm.h"
#include "linux/slab.h"

/*
 * TestCase_vmalloc.
 */
void TestCase_vmalloc(void)
{
	unsigned int addr;
	struct rb_node *node;
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;

	addr = vmalloc(PAGE_SIZE);

	mm_debug("ADDR %p\n",(void *)(unsigned long)addr);
	addr = vmalloc(PAGE_SIZE);
	mm_debug("ADDR %p\n",(void *)(unsigned long)addr);
	addr = vmalloc(PAGE_SIZE);
	mm_debug("ADDR %p\n",(void *)(unsigned long)addr);
	addr = vmalloc(PAGE_SIZE);
	mm_debug("ADDR %p\n",(void *)(unsigned long)addr);

	/* Trave all node */
	for(node = rb_first(&vmap_area_root) ; node ; node = rb_next(node)) {
		struct vmap_area *va;
		struct vm_struct *area;

		va = rb_entry(node,struct vmap_area,rb_node);
		area = va->private;
		mm_debug("VA %p\n",(void *)(unsigned long)va->va_start);
		mm_debug("PA %p\n",(void *)(unsigned long)area->phys_addr);
	}
	pgd = pgd_offset(&init_mm,addr);
	pmd = pmd_offset(pgd,addr);
	pte = pte_offset_kernel(pmd,addr);
	mm_debug("PGD %p PMD %p PTE %p\n",pgd,pmd,pte);
	M_show(__pa(pte),__pa(pte) + 20);
	vfree(addr);
	M_show(__pa(pte),__pa(pte) + 20);
}

/*
 * TestCase_vmalloc_PageTable.
 */
void TestCase_vmalloc_PageTable(void)
{
	unsigned int address;
	unsigned int find_address;
	unsigned int *area;
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	struct page *page;

	address = vmalloc(PAGE_SIZE);

	mm_debug("Address %p\n",(void *)(unsigned long)address);

	pgd = pgd_offset(&init_mm,address);
	mm_debug("PGD %p\n",pgd);
	mm_debug("PGD_VAL %p\n",(void *)pgd_val(pgd));
	pmd = pmd_offset(pgd,address);
	mm_debug("PMD %p\n",pmd);
	mm_debug("PMD_VAL %p\n",(void *)pmd_val(pmd));
	pte = pte_offset_kernel(pmd,address);
	mm_debug("PTE %p\n",pte);
	page = pmd_page(pmd);
	PageFlage(page,"A");
	pte = pte_offset_map(pmd,address);
	mm_debug("PPTE %p\n",pte);
	page = pte_page(pte);
	find_address = page_to_phys(page);
	mm_debug("UserAddress %p\n",(void *)(unsigned long)find_address);
	mm_debug("Find physical address  %p\n",vaddr_to_phys(address));
	area = phys_to_mem(vaddr_to_phys(address));
	area[1] = 0xF4454;
	mm_debug("AREA Data %p\n",area[1]);
	vfree(address);
}

/*
 * Calculate physical page number.
 */
void TestCase_PHYS_NUM(void)
{
#define NUM_NUMA 1
	struct pglist_data *pgdat;
	struct zonelist *zonelist;
	struct zoneref *zrf;
	struct zone *zone;
	struct page *page;
	int i;
	int high_idx[NUM_NUMA] = { 1 };


	for(i = 0 ; i < NUM_NUMA ; i++) {
		pgdat = NODE_DATA(i);
		zonelist = pgdat->node_zonelists;

		for_each_zone_zonelist(zone,zrf,zonelist,high_idx[i])
			mm_debug("Zone %s start_pfn %p start_phys %p\n",
					zone->name,zone->zone_start_pfn,
					pfn_to_phys(zone->zone_start_pfn));

	}
}
