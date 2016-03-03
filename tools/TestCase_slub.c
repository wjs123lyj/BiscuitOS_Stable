/*
 * This file use to debug Slub Allocator.
 * 
 * Create By: Buddy@2016-2-25
 */
#include "linux/kernel.h"
#include "linux/slab.h"
#include "linux/mm.h"
#include "linux/cpumask.h"
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
 * TestCase_slab_alloc1() - Let unative slab page join to list of
 * kmem_cache->node[0]->partial.
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

/*
 * TestCase_kmem_cache_shrink - Need more debug after debug Buddy Allocator.
 */
void TestCase_kmem_cache_shrink(void)
{
#define NUM_SLAB_PAGE 3
#define OBJ_SIZE      56
	struct kmem_cache *kmem_cache_test;
	struct test_struct {
		char array[OBJ_SIZE];
	} **objects;
	struct page *slab_page[NUM_SLAB_PAGE];
	struct page *page;
	unsigned int inuse_objects;
	unsigned int first_slab_page_objects;
	unsigned int page_mask;
	int i,j;

	/* Create the kmem_cache */
	kmem_cache_test = kmem_cache_create(__func__,
			sizeof(struct test_struct),8,0,NULL);
	KmemCache(kmem_cache_test,__func__);
	
	/* Calcuate the value for loop */
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

	objects = (struct test_struct **)kmalloc((NUM_SLAB_PAGE *
			oo_objects(kmem_cache_test->oo) - inuse_objects) *
			sizeof(struct test_struct **),GFP_KERNEL);

	/* Get a lot of objects from different slab page */
	for(i = 0 ; i < first_slab_page_objects ; i++)
		objects[i] = kmem_cache_alloc(kmem_cache_test,0);
	slab_page[0] = virt_to_page(__va(mem_to_phys(
					objects[0])));

	for(i = 1 ; i < NUM_SLAB_PAGE ; i++) {
		for(j = 0 ; j < oo_objects(kmem_cache_test->oo) ; j++)
			objects[i * oo_objects(kmem_cache_test->oo) + j - 
				inuse_objects] = kmem_cache_alloc(kmem_cache_test,0);
		slab_page[i] = virt_to_page(__va(mem_to_phys(
						objects[i * oo_objects(kmem_cache_test->oo) -
							inuse_objects])));
	}

	/* Pagelist view */
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++)
		PageFlage(slab_page[i],"SlabPageView");

	/* Check the list of kmem_cache->node[0]->partial */
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"First SlabList");

	/* Let unative slab page join to list of kmem_cache->node[0]->partial */
	for(i = 0 ; i < NUM_SLAB_PAGE - 1 ; i++)
		kmem_cache_free(kmem_cache_test,objects[i * 
				oo_objects(kmem_cache_test->oo) - inuse_objects]);

	/* Check the partial list */
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"2th SlabList");

	/* 
	 * Remove empty slab from the partial list and sort the remaining slabs
	 * by the number of items in use .
	 */
	PageFlage(kmem_cache_test->cpu_slab->page,"BeforeShrink");
	kmem_cache_shrink(kmem_cache_test);

	/* Add a TestCase: get a new object from partial list */
	objects[0] = kmem_cache_alloc(kmem_cache_test,0);
	PageFlage(virt_to_page(__va(mem_to_phys(objects[0]))),"Partial Slab");
	PageFlage(kmem_cache_test->cpu_slab->page,"After-Shrink");

	/* Check partial list */
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"3th SlabList");

	/* Previous TestCase complete.. */
	kmem_cache_free(kmem_cache_test,objects[0]);

	/* Pagelist view */
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++)
		PageFlage(slab_page[i],"PGViewShrink");

	/* Free all objects */
	for(i = 1 ; i < first_slab_page_objects ; i++)
		kmem_cache_free(kmem_cache_test,objects[i]);

	/* Shrink and check partial list */
	kmem_cache_shrink(kmem_cache_test);
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"4th SlabList");
	
	/* Pagelist view */
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++)
		PageFlage(slab_page[i],"Fpage-shrink");

	for(i = 1 ; i < NUM_SLAB_PAGE - 1 ; i++)
		for(j = 1 ; j < oo_objects(kmem_cache_test->oo) ; j++)
			kmem_cache_free(kmem_cache_test,objects[
					i * oo_objects(kmem_cache_test->oo) + j - inuse_objects]);

	for(i = 0 ; i < oo_objects(kmem_cache_test->oo) ; i++)
		kmem_cache_free(kmem_cache_test,objects[(NUM_SLAB_PAGE - 1) * 
				oo_objects(kmem_cache_test->oo) + i - inuse_objects]);

	/* Pagelist view */
	for(i = 0 ; i < NUM_SLAB_PAGE ; i++)
		PageFlage(slab_page[i],"Final-shrink");

	kfree(objects);
	kmem_cache_destroy(kmem_cache_test);
	mm_debug("Test complete..\n");
	
#undef NUM_SLAB_PAGE
#undef OBJ_SIZE    
}

/*
 * TestCaes_flush_all - debug function flush_all().
 *   
 * If you want debug this function,please open macro SLUB_DEBUG_FLUSH_ALL 
 * in autoconfig.h
 *
 * Create by Buddy.D.Zhang
 */
void TestCase_flush_all(void)
{
#define NUM_SLAB_PAGE 3
#define OBJ_SIZE      1024
	struct kmem_cache *kmem_cache_test;
	struct test_struct {
		char array[OBJ_SIZE];
	} **objects;
	struct page *slab_page[NUM_SLAB_PAGE];
	struct page *page;
	unsigned int page_mask;
	unsigned int inuse_objects;
	unsigned int first_slab_page_objects;
	int i,j;
	int new_num;

	kmem_cache_test = kmem_cache_create(__func__,
			sizeof(struct test_struct),8,0,NULL);
	KmemCache(kmem_cache_test,__func__);

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

	objects = (struct test_struct **)kmalloc((NUM_SLAB_PAGE *
				oo_objects(kmem_cache_test->oo) - inuse_objects) *
				sizeof(struct test_struct **),GFP_KERNEL);

	/*
	 * TestCase0: Alloc an object from new kmem_cache,and use flush_all().
	 */
	mm_debug("TestCase0\n");
	objects[0] = kmem_cache_alloc(kmem_cache_test,0);
	slab_page[0] = virt_to_page(__va(mem_to_phys(objects[0])));
	
	PageFlage(slab_page[0],"TestCase0");
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"PartialListCase0");
	
	mm_debug("Bslab_page->inuse %d\n",slab_page[0]->inuse);
#ifndef SLUB_DEBUG_FLUSH_ALL
	flush_all(kmem_cache_test);
#endif
	mm_debug("Aslab_page->inuse %d\n",slab_page[0]->inuse);
	
	PageFlage(slab_page[0],"FlushCase0");
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"PartialFlush");
	
	kmem_cache_free(kmem_cache_test,objects[0]);
	PageFlage(slab_page[0],"Nofree");
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"Nouse");

	/*
	 * TestCase1: alloc a lot objects that from same slab page,and then
	 * the freelist of slab page is null.We free all objects to slab page,
	 * and none object in used.
	 */
	mm_debug("TestCase1\n");
	for(i = 0 ; i < first_slab_page_objects ; i++)
		objects[i] = kmem_cache_alloc(kmem_cache_test,0);
	slab_page[0] = virt_to_page(__va(mem_to_phys(objects[0])));

	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"TC1->partial");
	PageFlage(slab_page[0],"TC1->NewSlab");

	for(i = 0 ; i < first_slab_page_objects ; i++)
		kmem_cache_free(kmem_cache_test,objects[i]);

	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"TC1->freepartial");
	PageFlage(slab_page[0],"TC1->oldslab");

	mm_debug("BSlab_page->inuse %d\n",slab_page[0]->inuse);
#ifdef SLUB_DEBUG_FLUSH_ALL
	flush_all(kmem_cache_test);
#endif
	mm_debug("ASlab_page->inuse %d\n",slab_page[0]->inuse);

	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"TC1->flushall");
	PageFlage(slab_page[0],"TC1->FA");

	/* End of Testing,free alloc memory */
	kfree(objects);
	kmem_cache_destroy(kmem_cache_test);
	mm_debug("TestCase Complete...\n");

#undef NUM_SLAB_PAGE
#undef OBJ_SIZE
}

/*
 * TestCase_flush_all0 - 
 */
void TestCase_flush_all0(void)
{
#define NUM_SLAB_PAGE 3
#define OBJ_SIZE      344
	struct kmem_cache *kmem_cache_test;
	struct test_struct {
		char array[OBJ_SIZE];
	} **objects;
	struct page **slab_page;
	struct page *page;
	unsigned int page_mask;
	unsigned int first_slab_page_objects;
	unsigned int inuse_objects;
	unsigned int page_num;
	int i,j;

	kmem_cache_test = kmem_cache_create(__func__,
			sizeof(struct test_struct),8,0,NULL);
	KmemCache(kmem_cache_test,__func__);

	if(NUM_SLAB_PAGE < kmem_cache_test->min_partial) 
		page_num = kmem_cache_test->min_partial + 2;
	else
		page_num = NUM_SLAB_PAGE;

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

	objects = (struct test_struct **)kmalloc(((page_num * 
				oo_objects(kmem_cache_test->oo)) - inuse_objects) *
				sizeof(struct test_struct **),GFP_KERNEL);
	slab_page = (struct page **)kmalloc(page_num *
				sizeof(struct page **),GFP_KERNEL);

	/* Alloc object from slab page */
	for(i = 0 ; i < first_slab_page_objects ; i++)
		objects[i] = kmem_cache_alloc(kmem_cache_test,0);
	slab_page[0] = virt_to_page(__va(mem_to_phys(objects[0])));

	for(i = 1 ; i < page_num ; i++) {
		for(j = 0 ; j < oo_objects(kmem_cache_test->oo) ; j++)
			objects[i * oo_objects(kmem_cache_test->oo) + j - inuse_objects] =
				kmem_cache_alloc(kmem_cache_test,0);
		slab_page[i] = virt_to_page(__va(mem_to_phys(
						objects[i * oo_objects(kmem_cache_test->oo) -
						inuse_objects])));
	}

	/* Check slab page */
	for(i = 0 ; i < page_num ; i++) 
		PageFlage(slab_page[i],"TC0");
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"TC0->Partial");

	/* Print all objects */
	mm_debug("SlabPage:0\n");
	mm_debug("Object[%3d]%p\n|\n",0,(void *)(unsigned long)
			__va(mem_to_phys(objects[i])));
	mm_debug("Object[%3d]%p\n",first_slab_page_objects - 1
			,(void *)(unsigned long)(
				__va(mem_to_phys(objects[first_slab_page_objects - 1]))));

	for(i = 1 ; i < page_num ; i++) {
		mm_debug("SlabPage:%d\n",i);
		mm_debug("Object[%3d]%p\n|\n",i * oo_objects(kmem_cache_test->oo) - 
				inuse_objects,(void *)(unsigned long)(
					__va(mem_to_phys(objects[
						i * oo_objects(kmem_cache_test->oo) -
						inuse_objects]))));
		mm_debug("Object[%3d]%p\n",(i + 1) * oo_objects(kmem_cache_test->oo) -
				inuse_objects - 1,(void *)(unsigned long)(
					__va(mem_to_phys(objects[
						(i + 1) * oo_objects(kmem_cache_test->oo) - 
						inuse_objects - 1]))));
	}

	/* Free all objects */
	for(i = 0 ; i < page_num * oo_objects(kmem_cache_test->oo) - 
			inuse_objects ; i++)
		kmem_cache_free(kmem_cache_test,objects[i]);

	for(i = 0 ; i < page_num ; i++) 
		PageFlage(slab_page[i],"TCE0");
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"TCG0");

#ifdef SLUB_DEBUG_FLUSH_ALL
	flush_all(kmem_cache_test);
#endif

	for(i = 0 ; i < page_num ; i++)
		PageFlage(slab_page[i],"TCP");
	list_for_each_entry(page,&kmem_cache_test->node[0]->partial,lru)
		PageFlage(page,"TGG0");
	/* TestCase complete.. */
	kfree(slab_page);
	kfree(objects);
	kmem_cache_destroy(kmem_cache_test);
	mm_debug("Test Complete...\n");
#undef NUM_SLAB_PAGE
#undef OBJ_SIZE
}

#ifdef SLUB_DEBUG_CALCULATE_ORDER
int nr_cpu_ids = 1;
#endif
/*
 * TestCase_calculate_sizes - Testing the kernel how to calculate the value
 *                            for kmem_cache.
 *
 * We we calculate the alignment of kmem_cache,if ALIGN(size,align) is equal 
 * to exist size of kmem_cache,the kmem_cache will reuse this kmem_cache not
 * create a new kmem_cache.
 *
 * min_objects = (fls(nr_cpu_ids) + 1) * 4.
 * When we calculate the order of slab page,the minimum order:
 * order = fls(min_objects * size - 1) - PAGE_SHIFT.
 *
 * Create by Buddy.D.Zhang
 */
void TestCase_calculate_sizes(void)
{
#define OBJ_SIZE        33
	struct kmem_cache *kmem_cache_test;
	int i;
	unsigned long objsize = 1024;

	kmem_cache_test = kmem_cache_create(__func__,OBJ_SIZE,
			8,0,NULL);
	KmemCache(kmem_cache_test,__func__);

	/*
	 * TestCase0: slab_order()
	 * 1. waste 1/16 memory.
	 */
#ifdef SLUB_DEBUG_SLAB_ORDER
	while(objsize) {
		mm_debug("Obj %6p slab_order %p\n",(void *)objsize,
				(void *)(unsigned long)slab_order(objsize,8,3,16));
		objsize >>= 1;
	}
#endif
	/*
	 * TestCase1: Calculate_order() full test!
	 * We must define macro SLUB_DEBUG_CALCULATE_ORDER and set
	 * nr_cpu_ids as 2,4,8,16,32...
	 */
#ifdef SLUB_DEBUG_CALCULATE_ORDER
	mm_debug("min_objects %p\n",(void *)(unsigned long)(
			(4 * (fls(nr_cpu_ids) + 1))));
	objsize = 4096;
	while(objsize) {
		mm_debug("Size %5p order %p\n",(void *)(unsigned long)objsize,
				(void *)(unsigned long)calculate_order(objsize));
		objsize >>= 1;
	}
#endif

	kmem_cache_destroy(kmem_cache_test);
	mm_debug("Test Complete.....\n");
#undef OBJ_SIZE
}

#ifdef SLUB_DEBUG_ALLOCATE_SLAB
extern struct page *allocate_slab(struct kmem_cache *,gfp_t,int);
#define for_each_object(__p,__s,__addr,__objects) \
	for(__p = (__addr) ; __p < (__addr) + (__objects) * (__s)->size; \
			__p += (__s)->size)
#endif
/**
 * TestCase_allocate_slab -
 */
void TestCase_allocate_slab(void)
{
#define OBJ_SIZE 135
	struct kmem_cache *kmem_cache_test;
	struct test_struct {
		char array[OBJ_SIZE];
	} *objects;
	struct page *page;
	void *start;
	void *last;
	void *p;

	kmem_cache_test = kmem_cache_create(__func__,OBJ_SIZE,8,0,NULL);
	KmemCache(kmem_cache_test,__func__);
	
	/*
	 * TestCase0:: PG_acall allocate_slab()
	 */
#ifdef SLUB_DEBUG_ALLOCATE_SLAB
	page = allocate_slab(kmem_cache_test,0,0);
#endif

	kmem_cache_destroy(kmem_cache_test);
	mm_debug("Test complete...\n");
#undef OBJ_SIZE
}
