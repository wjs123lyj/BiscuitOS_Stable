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
	mm_debug("Find physical address  %p\n",
			(void *)(unsigned long)vaddr_to_phys(address));
	area = phys_to_mem(vaddr_to_phys(address));
	area[1] = 0xF4454;
	mm_debug("AREA Data %p\n",(void *)(unsigned long)area[1]);
	vfree(address);
}

/*
 * TestCase_vmwrite_vread
 */
void TestCase_vwrite_vread(void)
{
	unsigned int vaddr;
	char *write_buf;
	char *read_buf;
	unsigned int write_addr;
	unsigned int read_addr;
	unsigned int use_addr;
	int count = 20;
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;
	struct page *page;

	vaddr = vmalloc(PAGE_SIZE);

	write_buf = (char *)kmalloc(40,GFP_KERNEL);
	read_buf  = (char *)kmalloc(40,GFP_KERNEL);

	write_addr = vaddr;
	read_addr = write_addr;

	pgd = pgd_offset_k(vaddr);
	if(!pgd_none(pgd)) {
		pmd = pmd_offset(pgd,vaddr);
		if(!pmd_none(pmd)) {
			pte = pte_offset_kernel(pmd,vaddr);
			if(!pte_none(pte)) {
				page = pte_page(pte);
				use_addr = pfn_to_phys(page_to_pfn(page));
				goto found;
			}
		}
	}

	return;
found:
	/* Prepare test data */
	memcpy(write_buf,"Buddy_Zhang",sizeof("Buddy_Zhang"));
	vwrite(write_buf,write_addr,sizeof("Buddy_Zhang"));
	mm_debug("Write %s\n",write_buf);
	vread(read_buf,read_addr,sizeof("Buddy_zhang"));
	mm_debug("Read  %s\n",read_buf);

	/* Finish Test */
	vfree(vaddr);
	kfree(read_buf);
	kfree(write_buf);
}
