/*
 * This file use to debug Per_Cpu_Page Allocator.
 *
 * Create by Buddy.D.Zhang
 */
#include "linux/kernel.h"
#include "linux/debug.h"
#include "linux/mmzone.h"
#include "linux/mm_types.h"
#include "linux/mm.h"
#include "linux/internal.h"
#include "linux/gfp.h"

/*
 * TestCae_PCP_normal - Get free page from Per_Cpu_Page.
 *                      When we get a physical page and kernel will return
 *                      page from Per_Cpu_Page Allocator.
 *                      If Per_Cpu_Page doesn't have free page and will
 *                      get from some pages from buddy system.
 *
 * Create by Buddy.D.Zhang
 */
void TestCase_PCP_normal(void) 
{
	struct pglist_data *pgdat;
	struct zonelist *zonelist;
	struct zone *zone;
	struct free_area *area;
	struct page *page;
	struct per_cpu_pages *pcp;
	struct list_head *list;
	int migratetype;
	int order,current_order;
	int cold;

	pgdat = NODE_DATA(0);
	zonelist = pgdat->node_zonelists;
	first_zones_zonelist(zonelist,0,NULL,&zone);
	migratetype = MIGRATE_MOVABLE;
	order = 0;

	pcp = &zone->pageset->pcp;
	list = &pcp->lists[migratetype];

	if(list_empty(list)) {
		int i;

		for(i = 0 ; i < pcp->batch ; i++) {
			for(current_order = order ; current_order < MAX_ORDER ;
					current_order++) {
				unsigned long size;

				area = &zone->free_area[current_order];
				if(list_empty(&area->free_list[migratetype]))
					continue;

				page = list_entry(area->free_list[migratetype].next,
							struct page,lru);
				list_del(&page->lru);
				rmv_page_order(page);
				area->nr_free--;

				size = 1 << current_order;
				while(order < current_order) {
					current_order--;
					area--;
					size >>= 1;
					list_add(&page[size].lru,
							&area->free_list[migratetype]);
					set_page_order(&page[size],current_order);
					area->nr_free++;
				}
				if(!PageBuddy(page))
					break;
			}
			if(cold != 0)
				list_add(&page->lru,list);
			else
				list_add_tail(&page->lru,list);
			set_page_private(page,migratetype);
			list = &page->lru;
		}
	}
	if(cold)
		page = list_entry(pcp->lists[migratetype].prev,struct page,lru);
	else
		page = list_entry(pcp->lists[migratetype].next,struct page,lru);
	list_del(&page->lru);
	set_page_private(page,migratetype);

	set_page_private(page,0);
	set_page_refcounted(page);

	PageFlage(page,"Found");
}

/*
 * TestCae_free_PCP - free a page to Per_Cpu_Page.
 */
void TestCase_free_pcp(void) 
{
	struct page *page;
	struct zone *zone;
	struct page *test_page;
	int migratetype;
	int cold = 0;
	struct per_cpu_pages *pcp;
	int i;

	page = alloc_page(GFP_KERNEL);
	migratetype = get_pageblock_migratetype(page);
	set_page_private(page,migratetype);
	zone = page_zone(page);
	pcp = &zone->pageset->pcp;

	for(i = 0 ; i < MIGRATE_PCPTYPES ; i++)
		list_for_each_entry(test_page,&pcp->lists[i],lru)
			PageFlage(test_page,"A");

	if(cold)
		list_add_tail(&page->lru,&pcp->lists[migratetype]);
	else
		list_add(&page->lru,&pcp->lists[migratetype]);
	pcp->count++;

	pcp->high = pcp->count;
	if(pcp->count >= pcp->high) {
		int to_free = pcp->batch - 1;
		int batch_free = 0;
		int migrate = 0;

		zone->all_unreclaimable = 0;
		zone->pages_scanned = 0;

		while(to_free) {
			struct list_head *list;

			do {
				batch_free++;
				if(++migrate == MIGRATE_PCPTYPES)
					migrate = 0;
				list = &pcp->lists[migrate];
			} while(list_empty(list));

			do {
				page = list_entry(list->prev,struct page,lru);

				list_del(&page->lru);
			} while(--to_free && --batch_free && !list_empty(list));
		}
		pcp->count -= pcp->batch;
	}
	for(i = 0 ; i < MIGRATE_PCPTYPES ; i++) 
		list_for_each_entry(page,&pcp->lists[i],lru)
			PageFlage(page,"B");
}

/*
 * TestCase_get_migratetype - get the migratetype of page.
 */
void TestCase_get_migratetype(void)
{
	struct zone *zone;
	struct page *page;
	unsigned long idx;
	unsigned long start_idx,value = 1;
	unsigned long migratetype = 0;

	page = alloc_page(GFP_KERNEL);
	zone = page_zone(page);
	idx = page_to_pfn(page);
	idx -= zone->zone_start_pfn;
	idx = (idx >> pageblock_order) * NR_PAGEBLOCK_BITS;

	for(start_idx = 0 ; start_idx < NR_PAGEBLOCK_BITS ;
			start_idx++ , value <<= 1) 
		if(test_bit(idx + start_idx,zone->pageblock_flags))
			migratetype |= value;
	
	mm_debug("migratetype %ld\n",migratetype);
}

/*
 * TestCase_free_pcppages_bulk.
 */
void TestCase_free_pcppages_bulk(void)
{
	struct page *page;
	struct zone *zone;
	struct per_cpu_pages *pcp;
	int i;

	page = alloc_page(GFP_KERNEL);
	zone = page_zone(page);
	pcp = &zone->pageset->pcp;
	
	for(i = 0 ; i < 3 ; i++)
		list_for_each_entry(page,&pcp->lists[i],lru)
			PageFlage(page,"A");

	if(pcp->count) {
		int batch_free;
		int to_free = pcp->count;
		int migratetype = 0;

		zone->all_unreclaimable = 0;
		zone->pages_scanned = 0;

		while(to_free) {
			struct list_head *list;

			do {
				++batch_free;
				if(++migratetype == MIGRATE_PCPTYPES)
					migratetype = 0;
				list = &pcp->lists[migratetype];
			} while(list_empty(list));

			do {
				page = list_entry(list->prev,struct page,lru);

				list_del(&page->lru);
			} while(--batch_free && --to_free && !list_empty(list));
		}
	}

	for(i = 0 ; i < 3 ; i++) 
		list_for_each_entry(page,&pcp->lists[i],lru)
			PageFlage(page,"B");
}
