/*
 * This file use to test Slub Allocator.
 * 
 * Create By: Buddy@2016-2-25
 */
#include "linux/kernel.h"
#include "linux/slab.h"
#include "linux/mm.h"

#define OO_SHIFT 16
#define OO_MASK  ((1 << OO_SHIFT) - 1)

/* Get the order of slab page */
static inline oo_order(struct kmem_cache_order_objects x)
{
	return x.x >> OO_SHIFT;
}
/* Get the object of slab page */
static inline oo_objects(struct kmem_cache_order_objects x)
{
	return x.x & OO_MASK;
}

/**
 * TestCase_slab_alloc0 - Alloc more slab page in a kmem_cache_node.
 *
 * For this testcase,I always alloc new object from kmem_cache,
 * and kernel alloc new slab page as soon as possible.But new slab 
 * page not connect to list of kmem_cache->kmem_cache_node->partial.
 * 
 * Buddy.D.Zhang
 * Error:(512-129,1024-9,256-65,64-16,32-9,16-5,)
 */
void TestCase_slab_alloc0(void)
{
#define NUM_SLAB_PAGE  4
#define OBJ_SIZE       1024
	struct kmem_cache *kmem_cache_test;
	struct test_struct {
		char array[OBJ_SIZE];
	} **objects;
	unsigned long first_slab_page_objects;
	unsigned long inuse_objects;
	unsigned int page_mask;
	struct page *slab_page[NUM_SLAB_PAGE];
	struct page *page;
	int i,j;

	kmem_cache_test = kmem_cache_create(__func__,
				sizeof(struct test_struct),0,0,NULL);
	KmemCache(kmem_cache_test,__func__);

	/* Alloc memory to the pointer array of objects */
	objects = (struct test_struct **)kmalloc(
			sizeof(struct test_struct **) * 
			oo_objects(kmem_cache_test->oo) * NUM_SLAB_PAGE,GFP_KERNEL);

	/* Get the mask of compand page */
	page_mask = (PAGE_SIZE << oo_order(kmem_cache_test->oo)) - 1;
	if(kmem_cache_test->cpu_slab->freelist) {
		unsigned int freelist = (unsigned int)(unsigned long)(
				kmem_cache_test->cpu_slab->freelist);

		inuse_objects = (freelist & page_mask) / ALIGN(OBJ_SIZE,8);
		first_slab_page_objects = DIV_ROUND_UP(
				(PAGE_SIZE << oo_order(kmem_cache_test->oo)) - 
				(freelist & page_mask),ALIGN(OBJ_SIZE,8)); 
	} else {
		inuse_objects = 0;
		first_slab_page_objects = oo_objects(kmem_cache_test->oo); 
	}

	/* Alloc object from first slab page */
	for(i = 0 ; i < first_slab_page_objects ;i++) 
		objects[i] = kmem_cache_alloc(kmem_cache_test,0);
	slab_page[0] = virt_to_page(
			__va(mem_to_phys(objects[0])));

	/* Alloc object from another slab page */
	for(i = 1 ; i < NUM_SLAB_PAGE ; i++) {
		for(j = 0 ; j < oo_objects(kmem_cache_test->oo) ; j++) {
			objects[i * oo_objects(kmem_cache_test->oo) + j - inuse_objects] =
				kmem_cache_alloc(kmem_cache_test,0);
		}
		slab_page[i] = virt_to_page(
				__va(mem_to_phys(objects[i * oo_objects(kmem_cache_test->oo) 
						- inuse_objects])));
	}

	/* Check slab page state */
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++)
		PageFlage(slab_page[i],"Slab Page");

	/* Check the list of kmem_cache->node[0]->partial */
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"Check list");

	/* For each object address */
	mm_debug("PageNum:0\n");
	for(i = 0 ; i < first_slab_page_objects ; i++)
		mm_debug("object[%2d]%p\n",i,(void *)(unsigned long)(
					__va(mem_to_phys(objects[i]))));

	for(i = 1 ; i < NUM_SLAB_PAGE ; i++) {
		mm_debug("PageNum:%d\n",i);
		for(j = 0 ; j < oo_objects(kmem_cache_test->oo) ; j++)
			mm_debug("object[%2d]%p\n",
					(int)(i * oo_objects(kmem_cache_test->oo) + 
						j - inuse_objects),
					(void *)(unsigned long)(
					__va(mem_to_phys(
							objects[i * oo_objects(kmem_cache_test->oo) + 
							j - inuse_objects]))));
	}

	/* The current cpu_slab state */
	if(kmem_cache_test->cpu_slab->freelist)
		mm_debug("kmem_cache->cpu_slab->freelist %p\n",(void *)
				__va(mem_to_phys(kmem_cache_test->cpu_slab->freelist)));
	else
		mm_debug("kmem_cache->cpu_slab->freelist NULL\n");

#undef OBJ_SIZE
#undef NUM_SLAB_PAGE
}

/**
 * TestCase_slab_alloc1() - Let slab page that flags isn't PG_active
 * join to list of kmem_cache->node[0]->partial.
 *
 * For this case,we have alloced some slab pages,and we'll free object 
 * from first slab page as much as possible.The page of free object 
 * will join to list of kmem_cache->node[0]->partial.
 *
 * Buddy.D.Zhang
 */
void TestCase_slab_alloc1(void)
{
#define NUM_SLAB_PAGE 2
#define OBJ_SIZE      256
	unsigned long num_objects = PAGE_SIZE / OBJ_SIZE;
	struct kmem_cache *kmem_cache_test;
	struct test_struct {
		char array[OBJ_SIZE];
	} *objects[num_objects * NUM_SLAB_PAGE];
	unsigned long first_slab_page_objects;
	unsigned long inuse_objects;
	struct page *slab_page[NUM_SLAB_PAGE];
	struct page *page;
	int i,j;

	/* Create kmem_cache */
	kmem_cache_test = kmem_cache_create(
			__func__,sizeof(struct test_struct),8,0,NULL);
	KmemCache(kmem_cache_test,__func__);

	if(kmem_cache_test->cpu_slab->freelist) {
		unsigned int freelist = (unsigned int)(unsigned long)(
				kmem_cache_test->cpu_slab->freelist);
		
		inuse_objects = (freelist & ~PAGE_MASK) / ALIGN(OBJ_SIZE,8);
		first_slab_page_objects = DIV_ROUND_UP(
				PAGE_SIZE - (freelist & ~PAGE_MASK),ALIGN(OBJ_SIZE,8));
	} else {
		first_slab_page_objects = PAGE_SIZE / ALIGN(OBJ_SIZE,8);
		inuse_objects = 0;
	}

	/* Alloc objects from first slab page */
	for(i = 0 ; i < first_slab_page_objects ; i++)
		objects[i] = kmem_cache_alloc(kmem_cache_test,0);
	slab_page[0] = virt_to_page(
			__va(mem_to_phys(objects[0])));

	/* Alloc objects from another slab page */
	for(i = 1 ; i < NUM_SLAB_PAGE ; i++) {
		for(j = 0 ; j < num_objects ; j++) 
			objects[i * num_objects + j - inuse_objects] = 
				kmem_cache_alloc(kmem_cache_test,0);
		/* Get the slab page forobject*/
		slab_page[i] = virt_to_page(
				__va(mem_to_phys(objects[i * num_objects - inuse_objects])));
	}

	/* Check slab page state */
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++)
		PageFlage(slab_page[i],"Slab Page");

	/* Check list of kmem_cache->node[0]->partial */
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"Check_page_partial");

	/* Free an object from first slab page */
	kmem_cache_free(kmem_cache_test,objects[0]);

	/* Check list of kmem_cache->node[0]->partial */
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"Free a object from first slab page");

	/* Free an object from the slab page with PG_active */
	kmem_cache_free(kmem_cache_test,objects[num_objects]);

	/* Check list of kmem_cache->node[0]->partial */
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru) 
		PageFlage(page,"Free a object from PG_active slab page");

#undef OBJ_SIZE
#undef NUM_SLAB_PAGE
}
