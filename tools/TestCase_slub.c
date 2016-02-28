/*
 * This file use to debug Slub Allocator.
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
 * TestCase_slab_alloc - Normal operation of alloc memory from 
 * the Slub Allocator.
 *
 * Create by Buddy.D.Zhang
 */
void TestCase_slab_alloc(void)
{
#define OBJ_SIZE 178
	struct kmem_cache *kmem_cache_test;
	struct test_struct {
		char array[OBJ_SIZE];
	} *objects;

	/* Create kmem_cache */
	kmem_cache_test = kmem_cache_create(__func__,
			sizeof(struct test_struct),8,0,NULL);
	KmemCache(kmem_cache_test,__func__);

	/* Alloc memory from kmem cache */
	objects = kmem_cache_alloc(kmem_cache_test,0);

	/* Use objects */
	objects->array[0] = 'A';
	mm_debug("Slub object:%c objsize %p\n",objects->array[0],
			(void *)(unsigned long)sizeof(*objects));

	/* Free memory to kmem cache */
	kmem_cache_free(kmem_cache_test,objects);

	/* Destroy the kmem_cache */
	kmem_cache_destroy(kmem_cache_test);
#undef OBJ_SIZE
}

/*
 * TestCase_kmalloc_kfree - Alloc memory from kmalloc and free memory
 * via kfree.
 *
 * Create by Buddy.D.Zhang
 */
void TestCase_kmalloc_kfree(void)
{
#define OBJ_SIZE   43
	struct test_struct {
		char array[OBJ_SIZE];
	} *objects;

	/* Alloc memory for objects from kmalloc */
	objects = (struct test_struct *)kmalloc(
			sizeof(struct test_struct),GFP_KERNEL);

	/* Use object */
	objects->array[0] = 'G';
	mm_debug("objects->array[0] %c objsize %p\n",objects->array[0],
			(void *)(unsigned long)sizeof(*objects));

	/* Free objects to kmalloc */
	kfree(objects);
#undef OBJ_SIZE
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

	/* Alloc memory to the pointer array of objects */
	objects = (struct test_struct **)kmalloc(
			sizeof(struct test_struct **) * 
			(oo_objects(kmem_cache_test->oo) * NUM_SLAB_PAGE -
			 inuse_objects),GFP_KERNEL);

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

	/* End of test and free all memory */
	for(i = 0 ; i < first_slab_page_objects ; i++)
		kmem_cache_free(kmem_cache_test,objects[i]);
	for(i = 1 ; i < NUM_SLAB_PAGE ; i++)
		for(j = 0 ; j < oo_objects(kmem_cache_test->oo) ; j++)
			kmem_cache_free(kmem_cache_test,
					objects[i * oo_objects(kmem_cache_test->oo) +
					j - inuse_objects]);

	kfree(objects);

	kmem_cache_destroy(kmem_cache_test);

	mm_debug("Test Complete\n");
#undef OBJ_SIZE
#undef NUM_SLAB_PAGE
}

/**
 * TestCase_slab_alloc1() - Let slab page that flags isn't PG_active
 * join to list of kmem_cache->node[0]->partial.
 *
 * For this case,we will alloc some objects that from different slab page,
 * and we will free an object that from an unactive slab page,we will trace 
 * the list of kmem_cache->node[0]->partial and the flage of slab page.
 * 
 * If we free an object to an unactive slab page,the slab page isn't equal 
 * to kmem_cache->cpu_slab->page.
 * 1. If the "inuse" of slab page isn't equal to 0 and the "freelist" of 
 *    slab is equal to NULL,the slab page will be added to list of 
 *    kmem_cache->node[0]->partial.
 * 2. If the "inuse" of slab page is equal to 0 that object is all free
 *    for this slab page while the "freelist" of slab page is empty,
 *    kernel will free this slab page to the PCP Allocator or Buddy Allocator.
 * 
 * Bug0:
 *     1. For this testcase,you shuld comfire that the size of 
 *        "sizeof(**objects)" doesn't equal to "OBJ_SIZE" when 
 *        kmem_cache->cpu_slab->free isn't NULL.If equal that
 *        kmem_cache_create() and kmalloc() will get objects from 
 *        same kmem_cache!For this testcaes,kmalloc() was called after
 *        kmem_cache_create() that we can't get correct vale of "inuse_objects"!
 *     2. I won't fixup this bug,it's good case to debug!
 *     3. For this bug,we can't get correct value of "inuse_objects" and
 *        we alloc some objects from other slab page when we want alloc
 *        some object from current slab_page.
 *     4. Bug-value:NUM_SLAB_PAGE : >= 9     
 *                  OBJ_SIZE:       1024  
 *     5. How to fixup?
 *            ALIGN(sizeof(void *) * oo_objects(kmem_cache->oo) * 
 *                  NUM_SLAB_PAGE,8) < OBJ_SIZE
 *
 * Create by Buddy.D.Zhang
 */
void TestCase_slab_alloc1(void)
{
#define NUM_SLAB_PAGE 8
#define OBJ_SIZE      1024
	struct kmem_cache *kmem_cache_test;
	struct test_struct {
		char array[OBJ_SIZE];
	} **objects;
	struct page *slab_page[NUM_SLAB_PAGE];
	struct page *page;
	unsigned int inuse_objects;
	unsigned int first_slab_page_objects;
	unsigned long page_mask;
	int i,j;

	/* Create kmem_cache */
	kmem_cache_test = kmem_cache_create(__func__,
			sizeof(struct test_struct),8,0,NULL);
	KmemCache(kmem_cache_test,__func__);

	page_mask = ((PAGE_SIZE << oo_order(kmem_cache_test->oo)) - 1);
	/* Calculate the number of used objects in first slab page.*/
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

	/* Alloc memory to pointer that point struct test_struct. */
	objects = (struct test_struct **)kmalloc((NUM_SLAB_PAGE *
			oo_objects(kmem_cache_test->oo) - inuse_objects) *
			sizeof(struct test_struct **),GFP_KERNEL);

	/* Alloc objects from kmem_cache */
	for(i = 0 ; i < first_slab_page_objects ; i++)
		objects[i] = kmem_cache_alloc(kmem_cache_test,0);
	slab_page[0] = virt_to_page(__va(mem_to_phys(
					objects[0])));

	for(i = 1 ; i < NUM_SLAB_PAGE ; i++) {
		for(j = 0 ; j < oo_objects(kmem_cache_test->oo) ; j++)
			objects[i * oo_objects(kmem_cache_test->oo) + j - inuse_objects] =
				kmem_cache_alloc(kmem_cache_test,0);
		slab_page[i] = virt_to_page(__va(mem_to_phys(
						objects[i * oo_objects(kmem_cache_test->oo) - 
						inuse_objects])));
	}

	for(i = 0 ; i < NUM_SLAB_PAGE ; i++)
		PageFlage(slab_page[i],"SlabPage");

	/* Check partial list */
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"partial");

	/* Print all address of objects */
	mm_debug("PageNum:0\n");
	for(i = 0 ; i < first_slab_page_objects ; i++)
		mm_debug("Objects[%2d]%p\n",i,(void *)(unsigned long)(
					__va(mem_to_phys(objects[i]))));
	for(i = 1 ; i < NUM_SLAB_PAGE ; i++) {
		mm_debug("PageNum:%d\n",i);
		for(j = 0 ; j < oo_objects(kmem_cache_test->oo) ; j++)
			mm_debug("Objects[%2d]%p\n",j - inuse_objects + i * 
					oo_objects(kmem_cache_test->oo),(void *)(unsigned long)(
						__va(mem_to_phys(
							objects[i * oo_objects(kmem_cache_test->oo) + 
							j - inuse_objects]))));
	}

	/* Free an object alloc from slab page that doesn't contain PG_active */
	kmem_cache_free(kmem_cache_test,objects[0]);

	/* Ckeck partial list */
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"FreePartial");
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++)
		PageFlage(slab_page[i],"Free");

	/* Free all memory */
	for(i = 1 ; i < NUM_SLAB_PAGE * oo_objects(kmem_cache_test->oo) -
			inuse_objects ; i++)
		kmem_cache_free(kmem_cache_test,objects[i]);

	kfree(objects);
	kmem_cache_destroy(kmem_cache_test);
	 
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++)
		PageFlage(slab_page[i],"End");
	mm_debug("Test Complete\n");

#undef OBJ_SIZE
#undef NUM_SLAB_PAGE
}
