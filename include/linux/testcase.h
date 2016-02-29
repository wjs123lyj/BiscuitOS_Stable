#ifndef _TESTCASE_H_
#define _TESTCASE_H_

extern void TestCase_slab_alloc(void);
extern void TestCase_slab_alloc0(void);
extern void TestCase_slab_alloc1(void);
extern void TestCase_kmalloc_kfree(void);
extern void TestCase_kmem_cache_shrink(void);
extern void TestCase_flush_all(void);
extern void TestCase_flush_all0(void);
extern void TestCase_calculate_sizes(void);
#endif
