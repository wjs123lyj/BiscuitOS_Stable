/*
 * This file use to test function in some case.
 * 
 * Create By: Buddy@2016-2-25
 */
#include "linux/kernel.h"
#include "linux/slab.h"
#include "linux/mm.h"


/**
 * TestCase_slab_alloc0 - Alloc more slab page in a kmem_cache_node.
 *
 * For this testcase,I always alloc new object from kmem_cache,
 * and kernel alloc new slab page as soon as possible.But new slab 
 * page not connect to list of kmem_cache->kmem_cache_node->partial.
 * 
 * Buddy.D.Zhang
 */
void TestCase_slab_alloc0(void)
{
#define NUM_SLAB_PAGE  3
#define OBJ_SIZE       51
	unsigned long num_objects = PAGE_SIZE / OBJ_SIZE;
	struct kmem_cache *kmem_cache_test;
	struct test_struct {
		char array[OBJ_SIZE];
	} *objects[NUM_SLAB_PAGE * num_objects];
	struct page *slab_page[NUM_SLAB_PAGE];
	struct page *page;
	int i,j;

	kmem_cache_test = kmem_cache_create(__func__,
				sizeof(struct test_struct),0,0,NULL);

	mm_debug("Kmem_cache: %s\n",kmem_cache_test->name);
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++) {
		for(j = 0 ; j < num_objects ; j++) 
			objects[i * num_objects + j] =
				kmem_cache_alloc(kmem_cache_test,0);
		slab_page[i] = virt_to_page(
				__va(mem_to_phys(objects[i *num_objects])));
		PageFlage(slab_page[i],"Alloc Page");
	}

	/* Check the list of kmem_cache->node[0]->partial */
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"Check list");

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
	struct page *slab_page[NUM_SLAB_PAGE];
	struct page *page;
	int i,j;

	/* Create kmem_cache */
	kmem_cache_test = kmem_cache_create(
			__func__,sizeof(struct test_struct),0,0,NULL);

	/* Alloc NUM_SLAB_PAGE slab page */
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++) {
		for(j = 0 ; j < num_objects ; j++) 
			objects[i * num_objects + j] = 
				kmem_cache_alloc(kmem_cache_test,0);
		/* Get the slab page forobject*/
		slab_page[i] = virt_to_page(
				__va(mem_to_phys(objects[i * num_objects])));
		PageFlage(slab_page[i],"Slab_page");
	}
	
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

	/* End of test and free remain memory */
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++)
		for(j = 1 ; j < num_objects ; j++)
			kmem_cache_free(kmem_cache_test,
					objects[i * num_objects + j]);

#undef OBJ_SIZE
#undef NUM_SLAB_PAGE
}
