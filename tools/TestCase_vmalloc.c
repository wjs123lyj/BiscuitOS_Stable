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
	unsigned long addr;

	addr = vmalloc(PAGE_SIZE);

	mm_debug("ADDR %p\n",(void *)addr);
}

