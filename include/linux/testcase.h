#ifndef _TESTCASE_H_
#define _TESTCASE_H_

/* Slub Allocator */
extern void TestCase_slab_alloc(void);
extern void TestCase_slab_alloc0(void);
extern void TestCase_slab_alloc1(void);
extern void TestCase_kmalloc_kfree(void);
extern void TestCase_kmem_cache_shrink(void);
extern void TestCase_flush_all(void);
extern void TestCase_flush_all0(void);
extern void TestCase_calculate_sizes(void);
extern void TestCase_allocate_slab(void);

/* Buddy Allocator */
extern TestCase_Buddy_normal(void);
extern TestCase_alloflags_to_migratetype(void);
extern TestCase_GFP(void);
extern TestCase_GFP_ZONE(void);
extern TestCase_zonelist(void);
extern TestCase_WMARK(void);
extern TestCase_PCP(void);
#endif
