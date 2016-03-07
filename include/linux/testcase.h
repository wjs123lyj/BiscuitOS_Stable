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
extern TestCase_rmqueu_smallest(void);
extern TestCase_page_order(void);
extern TestCase_Get_Buddy_Page(void);
extern TestCase_fallback(void);
extern TestCase_MovePage(void);
extern TestCase_rmqueue_fallback(void);
extern TestCase_pageblock_flage_group(void);
extern TestCase_PageBlock(void);

/* Per_Cpu_Page Allocator */
extern TestCase_PCP_normal(void);
extern TestCase_free_pcp(void);
extern TestCase_get_migratetype(void);
#endif
