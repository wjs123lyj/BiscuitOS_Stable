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
	vfree(addr);
}
