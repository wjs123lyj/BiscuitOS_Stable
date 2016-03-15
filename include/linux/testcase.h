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
extern void TestCase_Buddy_normal(void);
extern void TestCase_alloflags_to_migratetype(void);
extern void TestCase_GFP(void);
extern void TestCase_GFP_ZONE(void);
extern void TestCase_zonelist(void);
extern void TestCase_WMARK(void);
extern void TestCase_PCP(void);
extern void TestCase_rmqueu_smallest(void);
extern void TestCase_page_order(void);
extern void TestCase_Get_Buddy_Page(void);
extern void TestCase_fallback(void);
extern void TestCase_MovePage(void);
extern void TestCase_rmqueue_fallback(void);
extern void TestCase_pageblock_flage_group(void);
extern void TestCase_PageBlock(void);
extern void TestCase_free_one_page(void);
extern void TestCase_Find_Buddy(void);
extern void TestCase_page_is_buddy(void);
extern void TestCase_full_buddy(void);
extern void TestCase_SlowPath(void);
extern void TestCase_diff_alloc_page(void);
extern void TestCase_PageHighMem(void);

/* Per_Cpu_Page Allocator */
extern void TestCase_PCP_normal(void);
extern void TestCase_free_pcp(void);
extern void TestCase_get_migratetype(void);
extern void TestCase_free_pcppages_bulk(void);
extern void TestCase_full_pcp_buddy(void);

/* Vmalloc Allocator */
extern void TestCase_vmalloc(void);
extern void TestCase_vmalloc_PageTable(void);

/* Kmap Allocator */
extern void TestCase_kmap(void);
extern void TestCase_kmap_atomic(void);

/****** Kernel Lib ******/
/* RB tree */
extern void TestCase_RB_user(void);
/* HasH Algorithm*/
extern void TestCase_Hash(void);
/* Wait queue */
extern void TestCase_wait_queue(void);
#endif
