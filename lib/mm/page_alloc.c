#include "../../include/linux/kernel.h"
#include "../../include/linux/config.h"
#include "../../include/linux/mmzone.h"
#include "../../include/linux/highmem.h"
#include "../../include/linux/page.h"
#include "../../include/linux/debug.h"
#include "../../include/linux/nodemask.h"
#include "../../include/linux/pageblock-flags.h"
#include "../../include/linux/vmscan.h"
#include "../../include/linux/gfp.h"
#include "../../include/linux/vmstat.h"
#include "../../include/linux/mm.h"
#include "../../include/linux/atomic.h"
#include "../../include/linux/nommu.h"
#include "../../include/linux/page-flags.h"
#include "../../include/linux/bootmem.h"
#include "../../include/linux/cpu.h"
#include "../../include/linux/percpu.h"
#include "../../include/linux/ftree_event.h"
#include "../../include/linux/list.h"
#include "../../include/linux/internal.h"
#include "../../include/linux/memory_hotplug.h"
#include "../../include/linux/debug_locks.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Memory init data
 */
static unsigned long __meminitdata dma_reserve;
static unsigned long __meminitdata nr_kernel_pages;
static unsigned long __meminitdata nr_all_pages;
/*
 * Zonelist order in the kernel.
 */
#define ZONELIST_ORDER_DEFAULT   0
#define ZONELIST_ORDER_NODE      1
#define ZONELIST_ORDER_ZONE      2
static int current_zonelist_order = ZONELIST_ORDER_DEFAULT;
static char zonelist_order_name[3][8] = {"Default","Node","Zone"};

static struct trace_print_flags pageflag_names[] = {
	{ 1UL << PG_locked,         "locked"},
	{ 1UL << PG_error,          "error"},
	{ 1UL << PG_referenced,     "referenced"},
	{ 1UL << PG_uptodate,       "uptodate"},
	{ 1UL << PG_dirty,          "dirty"},
	{ 1UL << PG_lru,            "lru"},
	{ 1UL << PG_active,         "active"},
	{ 1UL << PG_slab,           "slab"},
	{ 1UL << PG_owner_priv_1,   "owner_priv_1"},
	{ 1UL << PG_arch_1,         "arch_1"},
	{ 1UL << PG_reserved,       "reserved"},
	{ 1UL << PG_private,        "private"},
	{ 1UL << PG_private_2,      "private_2"},
	{ 1UL << PG_writeback,      "writeback"},
#ifdef CONFIG_PAGEFLAGS_EXTENDED
	{ 1UL << PG_head,           "head"},
	{ 1UL << PG_tail,           "tail"},
#else
	{ 1UL << PG_compound,       "compound"},
#endif
	{ 1UL << PG_swapcache,      "swapcache"},
	{ 1UL << PG_mappedtodisk,   "mappedtodisk"},
	{ 1UL << PG_reclaim,        "reclaim"},
#ifdef CONFIG_MMU
	{ 1UL << PG_mlocked,        "mlocked"},
#endif
#ifdef CONFIG_ARCH_USES_PG_UNCACHED
	{ 1UL << PG_uncached,       "uncached"},
#endif
#ifdef CONFIG_MEMORY_FAILURE
	{ 1UL << PG_hwpoison,       "hwpoison"},
#endif
	{ -1UL,                      NULL   },
};

/*
 * pglist_data 
 */
struct pglist_data contig_pglist_data = {
#ifndef CONFIG_NO_BOOTMEM
	.bdata = &bootmem_node_data[0]
#endif
};

#define __ClearPageHead(x) 0
#define PageCompound(x) 0
#define set_pageblock_order(x) do {} while(0)
/*
 * The name of zone.
 */
static char *const zone_names[MAX_NR_ZONES] = {
#ifdef CONFIG_ZONE_DMA
	"DMA",
#endif
#ifdef CONFIG_ZONE_DMA32
	"DMA32",
#endif
	"NORMAL",
#ifdef CONFIG_HIGHMEM
	"HIGHMEM",
#endif
	"MOVABLE",
};
static inline void pgdat_resize_init(struct pglist_data *pgdat) {}
static inline void init_waitqueue_head(struct pglist_data *pgdat) {}
static inline void pgdat_page_cgroup_init(struct pglist_data *pgdat) {}
static inline void spin_lock_init(struct zone *zone) {}
static inline void zone_seqlock_init(struct zone *zone) {}
static inline void zone_pcp_init(struct zone *zone) {}
static inline void cpuset_init_mems_allowed(void) {}
static inline void cpuset_init_current_mems_allowed(void) {}
static int page_alloc_cpu_notify(void) {}
static inline void trace_mm_page_free_trace(struct page *p,int order) {}
static inline void kmemcheck_free_shadow(struct page *p,int order) {}

/*
 * No header.
 */
static int zone_wait_table_init(struct zone *zone,
		unsigned long zone_size_pages) 
{
	return 0;
}
/*
 * Return a pointer to the bitmap storing bits affecting a block of pages.
 */
static inline unsigned long *get_pageblock_bitmap(struct zone *zone,
		unsigned long pfn)
{
	return zone->pageblock_flags;
}
/*
 * Get the bitidx via pfn.
 */
static inline int pfn_to_bitidx(struct zone *zone,unsigned long pfn)
{
	pfn = pfn - zone->zone_start_pfn;
	return (pfn >> pageblock_order) * NR_PAGEBLOCK_BITS;
}
/*
 * Set the request group of flags for a pageblock_nr_pages block of pages
 */
void set_pageblock_flags_group(struct page *page,unsigned long flags,
		int start_bitidx,int end_bitidx)
{
	struct zone *zone;
	unsigned long *bitmap;
	unsigned long pfn,bitidx;
	unsigned long value = 1;

	zone = page_zone(page);
	pfn  = page_to_pfn(page);
	bitmap = get_pageblock_bitmap(zone,pfn);
	bitidx = pfn_to_bitidx(zone,pfn);
	VM_BUG_ON(pfn < zone->zone_start_pfn);
	VM_BUG_ON(pfn >= zone->zone_start_pfn + zone->spanned_pages);

	for(; start_bitidx <= end_bitidx ; start_bitidx++ , value <<= 1)
	{
		if(flags & value)
		{
			set_bit(bitidx + start_bitidx,bitmap);
		}
		else
		{
			clear_bit(bitidx + start_bitidx,bitmap);
		}
	}
}
/*
 * Set the migratetype of pageblcok
 */
static void set_pageblock_migratetype(struct page *page,int migratetype)
{
	if(unlikely(page_group_by_mobility_disabled))
		migratetype = MIGRATE_UNMOVABLE;
	set_pageblock_flags_group(page,(unsigned long)migratetype,
			PB_migrate,PB_migrate_end);
}
/*
 * Initially all pages are reserved - free ones are freed
 * up by free_all_bootmem() once the early boot process is
 * done.Non-atomic initialization,single-pass.
 */
void __meminit memmap_init_zone(unsigned long size,int nid,unsigned long zone,
		unsigned long start_pfn,enum memmap_context context)
{
	struct page *page;
	unsigned long end_pfn = start_pfn + size;
	unsigned long pfn;
	struct zone *z;

	if(highest_memmap_pfn < end_pfn - 1)
		highest_memmap_pfn = end_pfn - 1;

	z = &NODE_DATA(nid)->node_zones[zone];

	for(pfn = start_pfn ; pfn < end_pfn ; pfn++)
	{
		/*
		 * There can be holes in boot-time mem_map[]s
		 * handed to this function.They do not exist 
		 * on hotplugged memory.
		 */
		if(context == MEMMAP_EARLY)
		{
			if(!early_pfn_valid(pfn))
				continue;
			if(!early_pfn_in_nid(pfn,nid))
				continue;
		}
		page = pfn_to_page(pfn);
		set_page_links(page,zone,nid,pfn);
		mminit_verify_page_links(page,zone,nid,pfn);
		init_page_count(page);
		reset_page_mapcount(page);
		SetPageReserved(page);
		/*
		 * Mark the block movable so that blocks are reserved for
		 * movable at startup.This will force kernel allocations
		 * to reserve their block rather than leaking throughout
		 * the address space during boot when many long-lived
		 * kernel allocations are made.Later some blocks near
		 * the start are marked MIGRATE_RSERVE by
		 * setup_zone_migrate_reserve()
		 *
		 * bitmap is created for zone's valid pfn range.but memmap
		 * can be created for invalid pages (for alignment)
		 * check here not to call set_pageblock_migratetype() against
		 * pfn out of zone.
		 */
		if((z->zone_start_pfn <= pfn)
				&& (pfn < z->zone_start_pfn + z->spanned_pages)
				&& !(pfn & (pageblock_nr_pages - 1)))
			set_pageblock_migratetype(page,MIGRATE_MOVABLE);
		INIT_LIST_HEAD(&page->lru);
	}
}
#ifndef __HAVE_ARCH_MEMMAP_INIT
#define memmap_init(size,nid,zone,start_pfn)     \
	memmap_init_zone((size),(nid),(zone),(start_pfn),MEMMAP_EARLY)
#endif
/*
 * When CONFIG_HUGETLB_PAGE_SIZE_VARIABLE is not set,set_pageblock_order()
 * and pageback_default_order() are unused as pageblock_order is set
 * at comile-time.
 */
static inline int pageblock_default_order(unsigned int order)
{
	return MAX_ORDER - 1;
}
/*
 * Get spanned pages of zone.
 */
inline unsigned long __meminit zone_spanned_pages_in_node(int nid,
		unsigned long zone_type,unsigned long *zones_size)
{
	return zones_size[zone_type];
}
/*
 * Get the absent page of zone.
 */
inline unsigned long __meminit zone_absent_pages_in_node(int nid,
		unsigned long zone_type,unsigned long *zhole_size)
{
	return zhole_size[zone_type];
}
/*
 * Free list for buddy.
 */
static void __meminit zone_init_free_lists(struct zone *zone)
{
	int order,t;

	for_each_migratetype_order(order,t)
	{
		INIT_LIST_HEAD(&zone->free_area[order].free_list[t]);
		zone->free_area[order].nr_free = 0;
	}
}
/*
 * init current empty zone.
 */
__meminit int init_currently_empty_zone(struct zone *zone,
		unsigned long zone_start_pfn,
		unsigned long size,
		enum memmap_context context)
{
	struct pglist_data *pgdat = zone->zone_pgdat;
	int ret;
	
	ret = zone_wait_table_init(zone,size);
	if(ret)
		return ret;
	pgdat->nr_zones = zone_idx(zone) + 1;

	zone->zone_start_pfn = zone_start_pfn;

	mm_debug("memmap_init initialising map node %ld zone %lu"
			" pfns %p -> %p\n",
			pgdat->node_id,
			(unsigned long)zone_idx(zone),
			(void *)zone_start_pfn,(void *)(zone_start_pfn + size));

	/* Initialize the Buddy allocator */
	zone_init_free_lists(zone);

	return 0;
}
/*
 * Calculate the size of the zone->blockflags rounded to an unsigned long
 * Start by making sure zonesize is a mulitple of pageblock_order by rounding
 * up.Then use 1 NR_PAGEBLOCK_BITS worth of bits per pageblock,finally 
 * round what is now in bits to nearest long in bits,then return it in bytes.
 */
static unsigned long __init usemap_size(unsigned long zonesize)
{
	unsigned long usemapsize;

	usemapsize = roundup(zonesize,pageblock_nr_pages);
	usemapsize = usemapsize >> pageblock_order;
	usemapsize *= NR_PAGEBLOCK_BITS;
	usemapsize = roundup(usemapsize,8 * sizeof(unsigned long));

	return usemapsize / 8;
}
/*
 * setup usemap.
 */
static void __init setup_usemap(struct pglist_data *pgdat,
		struct zone *zone,unsigned long zonesize)
{
	unsigned long usemapsize = usemap_size(zonesize);

	zone->pageblock_flags = NULL;

	if(usemapsize)
		/*
		 * In order to simulate physcail address and virtual address,we lead 
		 * into
		 * virtual memory address.we can use virtual memory address directly.
		 */
		zone->pageblock_flags = (unsigned long *)(unsigned long)alloc_bootmem_node(
				pgdat,usemapsize);
}
/*
 * The core of free and init area.
 * - mark all pages reserved.
 * - mark all memory queue empty.
 * - clear the memory bitmaps.
 */
void __paginginit free_area_init_core(struct pglist_data *pgdat,
		unsigned long *zones_size,unsigned long *zhole_size)
{
	enum zone_type j;
	int nid = pgdat->node_id;
	unsigned long zone_start_pfn = pgdat->node_start_pfn;
	int ret;
	
	pgdat_resize_init(pgdat);
	pgdat->nr_zones = 0;
	init_waitqueue_head(pgdat);
	pgdat->kswapd_max_order = 0;
	pgdat_page_cgroup_init(pgdat);
	
	for(j = 0 ; j < MAX_NR_ZONES ; j++)
	{
		struct zone *zone = pgdat->node_zones + j;
		unsigned long size,realsize,memmap_pages;
		enum lru_list l;
		
		size = zone_spanned_pages_in_node(nid,j,zones_size);
		realsize = size - zone_absent_pages_in_node(nid,j,zhole_size);
		/*
		 * Adjust realsize so that it accounts for how much memory
		 * is used by this zone for memmap.This affects the watermark
		 * and per-cpu initialisations.
		 */
		memmap_pages = 
			PAGE_ALIGN(size * sizeof(struct page)) >> PAGE_SHIFT;
		if(realsize >= memmap_pages)
		{
			realsize -= memmap_pages;
			if(memmap_pages)
				mm_debug("%s zone: %lu pages used for memmap\n",
						zone_names[j],memmap_pages);
		} else
			mm_err("%s zone:%lu pages exceeds realsize %lu\n",
					zone_names[j],memmap_pages , realsize);
		/* Account for reserved pages */
		if(j == 0 && realsize > dma_reserve)
		{
			realsize -= dma_reserve;
			mm_debug("%s zone: %lu pages reserved\n",
					zone_names[0],dma_reserve);
		}
		/* Caculate the kernel pages and count all can use pages */
		if(!is_highmem_idx(j))
			nr_kernel_pages += realsize;
		nr_all_pages += realsize;
		/*
		 * initialize the zone.
		 */
		zone->spanned_pages = size;
		zone->present_pages = realsize;
		zone->name = zone_names[j];
		/* Initialize the spinlock of zone */
		spin_lock_init(zone); 
		/* Initialize the lru locked of zone */
		spin_lock_init(zone);
		/* Initialize the seqlock */
		zone_seqlock_init(zone);
		/* Setup the pgdat for zone */
		zone->zone_pgdat = pgdat;
		/* Initialize the pcp in zone */
		zone_pcp_init(zone);

		for_each_lru(l) 
		{
			INIT_LIST_HEAD(&zone->lru[l].list);
			zone->reclaim_stat.nr_saved_scan[l] = 0;
		}

		zone->reclaim_stat.recent_rotated[0] = 0;
		zone->reclaim_stat.recent_rotated[1] = 0;
		zone->reclaim_stat.recent_scanned[0] = 0;
		zone->reclaim_stat.recent_scanned[1] = 0;
		zap_zone_vm_stats(zone);
		zone->flags = 0;
		if(!size)
			continue;
		set_pageblock_order(pageblock_default_order());
		setup_usemap(pgdat,zone,size);
		
		ret = init_currently_empty_zone(zone,zone_start_pfn,
				size,MEMMAP_EARLY);
		memmap_init(size,nid,j,zone_start_pfn);
		zone_start_pfn += size;
	}
}

/*
 * Set the order of zonelist.
 */
static void set_zonelist_order(void)
{
	current_zonelist_order = ZONELIST_ORDER_ZONE;
}
/*
 * Check the highest zone.
 */
static inline void check_highest_zone(int k)
{
}
/*
 * Check whether zone has populated.
 */
inline int populated_zone(struct zone *zone)
{
	return (!!zone->present_pages);
}
/*
 * Set the zoneref for zone.
 */
static void zoneref_set_zone(struct zone *zone,struct zoneref *zoneref)
{
	zoneref->zone = zone;
	zoneref->zone_idx = zone_idx(zone);
}
/*
 * Builds allocation fallback zone lists.
 *
 * Add all populated zones of a node to the zonelist.
 */
static int build_zonelists_node(struct pglist_data *pgdat,
		struct zonelist *zonelist,int nr_zones,enum zone_type zone_type)
{
	struct zone *zone;

	zone_type++;

	do {
		zone_type--;
		zone = pgdat->node_zones + zone_type;
		if(populated_zone(zone)) 
		{
			zoneref_set_zone(zone,
					&zonelist->_zonerefs[nr_zones++]);
			check_highest_zone(zone_type);
		} 
	} while(zone_type);
	return nr_zones;
}
/*
 * build zonelist
 */
static void build_zonelists(struct pglist_data *pgdat)
{
	int node,local_node;
	enum zone_type j;
	struct zonelist *zonelist;

	local_node = pgdat->node_id;

	zonelist = &pgdat->node_zonelists[0];
	j = build_zonelists_node(pgdat,zonelist,0,MAX_NR_ZONES - 1);
	/*
	 * Now we build the zonelist so that it contains the zones
	 * of all the other nodes.
	 * We don't want to pressure a particular node,so when building
	 * the zones for node N,we make sure that the zones coming
	 * right after the local ones are those from 
	 * node N + 1 (modulo N)
	 */
	for(node = local_node + 1 ; node < MAX_NUMNODES ; node++)
	{
		if(!node_online(node))
			continue;
		j = build_zonelists_node(NODE_DATA(node),zonelist,j,
				MAX_NR_ZONES - 1);
	}
	for(node = 0 ; node < local_node ; node++)
	{
		if(!node_online(node))
			continue;
		j = build_zonelists_node(NODE_DATA(node),zonelist,j,MAX_NR_ZONES - 1);
	}
	zonelist->_zonerefs[j].zone = NULL;
	zonelist->_zonerefs[j].zone_idx = 0;
}
/*
 * non-NUMA variant of zonelist performance cache - just NULL zlcache_ptr.
 */
static void build_zonelist_cache(struct pglist_data *pgdat)
{
	pgdat->node_zonelists[0].zlcache_ptr = NULL;
}
/*
 * Get node id in UMA
 */
static inline int numa_node_id(void)
{
	return 0;
}
/*
 * Get the number of free pages of zone.
 */
static unsigned int nr_free_zone_pages(int offset)
{
	struct zoneref *z;
	struct zone *zone;

	/*
	 * Just pick one node,since fallback list is circular.
	 */
	unsigned int sum = 0;
	struct zonelist *zonelist = node_zonelist(numa_node_id(),GFP_KERNEL);
	
	for_each_zone_zonelist(zone,z,zonelist,offset)
	{
		unsigned long size = zone->present_pages;
		unsigned long high = high_wmark_pages(zone);

		if(size > high)
			sum += size - high;
	}
	return sum;
}
/*
 * Amount of free RAM allocatable within all zones.
 */
unsigned int nr_free_pagecache_pages(void)
{
	return nr_free_zone_pages(gfp_zone(GFP_HIGHUSER_MOVABLE));
}
/*
 * build all zonelist
 */
static __init_refok int __build_all_zonelist(void *data)
{
	int nid;
	int cpu;

	for_each_online_node(nid)
	{	
		struct pglist_data *pgdat = NODE_DATA(nid);
		/*
		 * Connect between zone and zonelist->zoneref.
		 */
		build_zonelists(pgdat);
		/*
		 * Set zonelist.zlcache.
		 */
		build_zonelist_cache(pgdat);
	}
	return 0;
}
/*
 * Called with zonelist_mutex held always
 * Unless system_state == SYSTEM_BOOTING.
 */
void build_all_zonelist(void *data)
{
	set_zonelist_order();
	
	if(system_state == SYSTEM_BOOTING)
	{
		__build_all_zonelist(NULL);
		mminit_verify_zonelist();
		cpuset_init_current_mems_allowed();
	}
	vm_total_pages = nr_free_pagecache_pages();
	/*
	 * Disable grouping by modify if the number of pages in the 
	 * system is too low to alloc the mechanism to work.It would be
	 * more accurate,but expensive to check per-zone.This check is 
	 * made on memory-hotadd so a system can start with mobilitu
	 * disable and enable it later
	 */
	if(vm_total_pages < (pageblock_nr_pages * MIGRATE_TYPES))
		page_group_by_mobility_disabled = 1;
	else
		page_group_by_mobility_disabled = 0;
	mm_debug("Built %i zonelist in %s order,mobility grouping %s."
			"Total pages : %ld\n",
			nr_online_nodes,
			zonelist_order_name[current_zonelist_order],
			page_group_by_mobility_disabled ? "off":"on",
			vm_total_pages);
}

void __init page_alloc_init(void)
{
	hotcpu_notifier(page_alloc_cpu_notify,0);
}
/*
 * Dump flags information of page.
 */
static void dump_page_flags(unsigned long flags)
{
	const char *delim = "";
	unsigned long mask;
	int i;

	mm_debug("page flags %p(",flags);

	/* remove zone id */
	flags &= (1UL << NR_PAGEFLAGS) - 1;

	for(i = 0 ; pageflag_names[i].name && flags ; i++)
	{
		mask = pageflag_names[i].mask;
		if((flags & mask) != mask)
			continue;

		flags &= ~mask;
		mm_debug("%s%s",delim,pageflag_names[i].name);
		delim = "|";
	}

	/* Check for left over flags */
	if(flags)
		mm_debug("%s %p",delim,flags);
	mm_debug(")\n");
}
/*
 * Dump information of page.
 */
void dump_page(struct page *page)
{
	mm_debug("page %p count %p mapcount %p mapping %p index %p\n",
			(void *)page,(void *)atomic_read(&page->_count),
			(void *)page_mapcount(page),
			(void *)page->mapping,(void *)page->index);
	dump_page_flags(page->flags);
}
/*
 * Bad page.
 */
static void bad_page(struct page *page)
{
	static unsigned long resume;
	static unsigned long nr_shown;
	static unsigned long nr_unshown;

	/* Don't complain about poisoned pages */
	if(PageHWPoison(page))
	{
		__ClearPageBuddy(page);
		return;
	}
	dump_page(page);
	dump_stack();
out:
	/* Leave bad fields for debug,except PageBuddy could make trouble */
	__ClearPageBuddy(page);
//	add_taint(TAINT_BAD_PAGE);
}
/*
 * Permit the bootmem allocator to evade page validation on high-order frees.
 */
void __meminit __free_pages_bootmem(struct page *page,unsigned int order)
{
	if(order == 0)
	{
		__ClearPageReserved(page);
		set_page_count(page,0);
		set_page_refcounted(page);
		__free_page(page);
	} else
	{
		int loop;

		prefetchw(page);
		for(loop = 0 ; loop < BITS_PER_LONG ; loop++)
		{
			struct page *p = &page[loop];

			if(loop + 1 < BITS_PER_LONG)
				prefetchw(p + 1);
			__ClearPageReserved(p);
			set_page_count(p,0);
		}
		
		set_page_refcounted(page);
		__free_pages(page,order);
	}
}
static inline int free_pages_check(struct page *page)
{
	if(unlikely(page_mapcount(page) |
				(page->mapping != NULL) |
				(atomic_read(&page->_count) != 0) |
				(page->flags & PAGE_FLAGS_CHECK_AT_FREE)))
	{
		bad_page(page);
		return 1;
	}
	if(page->flags & PAGE_FLAGS_CHECK_AT_PREP)
		page->flags &= ~PAGE_FLAGS_CHECK_AT_PREP;
	return 0;
}
/*
 * free page prepare
 */
static bool free_pages_prepare(struct page *page,unsigned int order)
{
	int i;
	int bad = 0;
	
	kmemcheck_free_shadow(page,order);

	if(PageAnon(page))
		page->mapping = NULL;
	for(i = 0 ; i < (1 << order) ; i++)
		bad += free_pages_check(page + i);
	if(bad)
		return false;

	if(!PageHighMem(page))
	{
		debug_check_no_locks_freed(page_address(page),PAGE_SIZE << order);
		debug_check_no_obj_freed(page_address(page),
				PAGE_SIZE << order);
	}
	arch_free_page(page,order);
	kernel_map_pages(page,1 << order , 0);

	return true;
}
/*
 * Return the request group of flags for the pageblock_nr_block of pages.
 */
unsigned long get_pageblock_flags_group(struct page *page,
		int start_bitidx,int end_bitidx)
{
	struct zone *zone;
	unsigned long *bitmap;
	unsigned long pfn,bitidx;
	unsigned long flags = 0;
	unsigned long value = 1;

	zone = page_zone(page);
	pfn  = page_to_pfn(page);
	bitmap = get_pageblock_bitmap(zone,pfn);
	bitidx = pfn_to_bitidx(zone,pfn);

	for(; start_bitidx <= end_bitidx ; start_bitidx++ , value <<= 1)
		if(test_bit(bitidx + start_bitidx,bitmap))
			flags |= value;

	return flags;
}
/*
 * clean up attempts to free and mlocked() page.
 * Page should not be on lru,so no need to fix that up.
 */
static inline void free_page_mlock(struct page *page)
{
	__dec_zone_page_state(page,NR_MLOCK);
//	__count_vm_event(UNEVICTABLE_MLOCKFREED);
}
static int page_is_consistent(struct zone *zone,struct page *page)
{
	if(!pfn_valid_within(page_to_pfn(page)))
		return 0;
	if(zone != page_zone(page))
		return 0;

	return 1;
}
static int page_outside_zone_boundaries(struct zone *zone,struct page *page)
{
	int ret = 0;
	unsigned seq;
	unsigned long pfn = page_to_pfn(page);

	do {
		seq = zone_span_seqbegin(zone);
		if(pfn >= zone->zone_start_pfn + zone->spanned_pages)
			ret = 1;
		else if(pfn < zone->zone_start_pfn)
			ret = 1;
	} while(zone_span_seqretry(zone,seq));

	return ret;
}
/*
 * Temporary debugging check for pages not lying within a given zone.
 */
static int bad_range(struct zone *zone,struct page *page)
{
	if(page_outside_zone_boundaries(zone,page))
		return 1;
	if(!page_is_consistent(zone,page))
		return 1;
	return 0;
}

static inline void set_page_order(struct page *page,int order)
{
	set_page_private(page,order);
	__SetPageBuddy(page);
}
static inline void rmv_page_order(struct page *page)
{
	__ClearPageBuddy(page);
	set_page_private(page,0);
}
/*
 * update ___split_huge_page_refcount if you change this function.
 */
static int destroy_compound_page(struct page *page,unsigned long order)
{
	int i;
	int nr_pages = 1 << order;
	int bad = 0;

	if(unlikely(compound_order(page) != order) ||
			unlikely(!PageHead(page)))
	{
		bad_page(page);
		bad++;
	}

	__ClearPageHead(page);

	for(i = 1 ; i < nr_pages ; i++)
	{
		struct page *p = page + i;

		if(unlikely(!PageTail(p) || (p->first_page != page)))
		{
			bad_page(page);
			bad++;
		}
		__ClearPageTail(p);
	}
	return bad;
}

/*
 * Locate the struct page for both the matching buddy in our
 * pair(buddy 1) and the combined O(n+1) page they form(page).
 *
 * 1) Any buddy B1 will haven an order O twin B2 which satisfies
 * the following equation:
 *  B2 = B1 ^ (1 << O)
 * For example,if the starting buddy(buddy2) is #8 its order
 * 1 buddy is #10:
 * B2 = 8 ^(1 << 1) = 8 ^ 2 = 10
 *
 * 2) Any buddy B will have an order O + 1 parent P which
 * satisfies the following equation:
 * P = B & ~(1 << O)
 *
 * Assumpption: *_mem_map is contiguous at least up to MAX_ORDER
 */
static inline unsigned long __find_buddy_index(unsigned long page_idx,
		unsigned long order)
{
	return page_idx ^ ( 1 << order);
}
/*
 * This function check whether a page is free && is the buddy
 * we can do coalesce a page and its buddy if
 * (a) the buddy is not in a hole &&
 * (b) the buddy is in the buddy system &&
 * (c) a page and its buddy have the same order &&
 * (d) a page and its buddy are in the same zone.
 *
 * For reording whether a page is in the buddy system,we set->_mapcount - 2.
 * Setting,clearing,and testing_mapcount -2 is serialized by zone->lock.
 *
 * For recording page's order,we use page_private(page).
 */
static inline int page_is_buddy(struct page *page,struct page *buddy,
		int order)
{
	if(!pfn_valid_within(page_to_pfn(buddy)))
		return 0;
	if(page_zone_id(page) != page_zone_id(buddy))
		return 0;
	if(PageBuddy(buddy) && page_order(buddy) == order)
	{
		VM_BUG_ON(page_count(buddy) != 0);
		return 1;
	}
	return 0;
}
/*
 * Freeing function for a buddy system allocator.
 * The concept of a buddy system is to maintain direct-mapped table
 * (containing bit values) for memory blocks of various "orders".
 * The bottom level table contains the map for the smallest allocatable
 * units of memory(here,pages),and each level above it describes
 * paris of units from the levels below,hence,"buddies".
 * At a high level,all that happens here is marking the table entry
 * at the bottom level available,and propagating the changes upward
 * as necessary,plus some accounting needed to play nicely with other
 * parts of the VM system.
 * At each level,we keep a list of pages,which are heads of continuous
 * free pages of length of(1 << order) and mark with _mapcount -2.Page's
 * order is recorded in page_private(page) field.
 * So when we are allocating or freeing one,we can derive the state of the
 * other.That is,if we allocate a small block.and both were
 * free,the remainder of the region must be split into blocks.
 * If a block is freed,and its buddy is also free,then this
 * triggers coalescing into a block of larger size.
 *
 */
static inline void __free_one_page(struct page *page,
		struct zone *zone,unsigned int order,int migratetype)
{
	unsigned long page_idx;
	unsigned long combined_idx;
	unsigned long buddy_idx;
	struct page *buddy;

	if(unlikely(PageCompound(page)))
		if(unlikely(destroy_compound_page(page,order)))
			return;

	VM_BUG_ON(migratetype == -1);

	page_idx = page_to_pfn(page) & ((1 << MAX_ORDER) - 1);

	VM_BUG_ON(page_idx & (( 1 << order) - 1));
	VM_BUG_ON(bad_range(zone,page));

	while(order < MAX_ORDER - 1)
	{
		buddy_idx = __find_buddy_index(page_idx,order);
		buddy = page + (buddy_idx - page_idx);
		if(!page_is_buddy(page,buddy,order))
			break;

		/* Our buddy is free.merge with if and move up one order */
		list_del(&buddy->lru);
		zone->free_area[order].nr_free--;
		rmv_page_order(buddy);
		combined_idx = buddy_idx & page_idx;
		page = page + (combined_idx - page_idx);
		page_idx = combined_idx;
		order++;
	}
	set_page_order(page,order);
	
	/*
	 * If this is not the largest possible page,check if the buddy
	 * of the next-highest order is free.If it is,it's possible
	 * that pages are being freed that will coalesce soon.In case,
	 * that is happing,add the free page to the tail of the list 
	 * so it's less likely to be used soon and more likely to be merged
	 * as a higher order page.
	 */
	if((order < MAX_ORDER - 2) && pfn_valid_within(page_to_pfn(buddy)))
	{
		struct page *higher_page,*higher_buddy;

		combined_idx = buddy_idx & page_idx;
		higher_page = page + (combined_idx - page_idx);
		buddy_idx = __find_buddy_index(combined_idx,order + 1);
		higher_buddy = page + (buddy_idx - combined_idx);
		if(page_is_buddy(higher_page,higher_buddy,order + 1))
		{
			list_add_tail(&page->lru,
					&zone->free_area[order].free_list[migratetype]);
			goto out;
		}
	}
	list_add(&page->lru,&zone->free_area[order].free_list[migratetype]);
out:
	zone->free_area[order].nr_free++;
}

static void free_one_page(struct zone *zone,struct page *page,int order,
		int migratetype)
{
//	spin_lock(&zone->lock);
	zone->all_unreclaimable = 0;
	zone->pages_scanned = 0;

	__free_one_page(page,zone,order,migratetype);
	__mod_zone_page_state(zone,NR_FREE_PAGES,1 << order);
//	spin_unlock(&zone->lock);
}
/*
 * Frees a number of pages from the PCP lists.
 * Assumes all pages on list are in same zone,and of same order.
 * count is the number of pages to free.
 *
 * If the zone was previously in an "all pages pinned" state then lock to 
 * see if this freeing clears that state.
 * 
 * And clear the zone's pages_scanned counter,to hold off the "all pages are
 * pinned" detection logic.
 */
static void free_pcppages_bulk(struct zone *zone,int count,
		struct per_cpu_pages *pcp)
{
	int migratetype = 0;
	int batch_free  = 0;
	int to_free     = count;

//	spin_lock(&zone->lock);
	zone->all_unreclaimable = 0;
	zone->pages_scanned = 0;

	while(to_free)
	{
		struct page *page;
		struct list_head *list;

		/*
		 * Remove pages from lists in a round-robin fashin.A
		 * batch_free count is maintained that is incremented when an
		 * empty list is encountered.This is so more pages are freed
		 * off fuller lists instead of spinning excessively around empty
		 * lists.
		 */
		do {
			batch_free++;
			if(++migratetype == MIGRATE_PCPTYPES)
				migratetype = 0;
			list = &pcp->lists[migratetype];
		} while(list_empty(list));

		do {
			page = list_entry(list->prev,struct page,lru);
			/* must delete as __free_one_page list manipulates */
			list_del(&page->lru);
			/* MIGRATE_MOVABLE list may include MIGRATE_RESERVEs */
			__free_one_page(page,zone,0,page_private(page));
		} while(--to_free && --batch_free && !list_empty(list));
	} 
	__mod_zone_page_state(zone,NR_FREE_PAGES,count);
//	spin_unlock(&zone->lock);
}
/*
 * Free a 0-order page
 * cold == 1 ? free cold page: free a hot page
 */
void free_hot_cold_page(struct page *page,int cold)
{
	struct zone *zone = page_zone(page);
	struct per_cpu_page *pcp;
	unsigned long flags;
	int migratetype;
	int wasMlocked = __TestClearPageMlocked(page);

	if(!free_pages_prepare(page,0))
		return;

	migratetype = get_pageblock_migratetype(page);
	set_page_private(page,migratetype);
//	loca_irq_save(flags);
	if(unlikely(wasMlocked))
		free_page_mlock(page);
//	__count_vm_event(PGFREE);
	/*
	 * We only track unmovable,reclaimable and movable on pcp lists.
	 * Free ISOLATE pages back to the allocator because they are being
	 * offlined but treat RESERVE as movable pages so we can get those
	 * areas back if necessary.Otherwise,we may have to free 
	 * excessively into the page allocator.
	 */
	if(migratetype >= MIGRATE_PCPTYPES)
	{
		if(unlikely(migratetype == MIGRATE_ISOLATE))
		{
			free_one_page(zone,page,0,migratetype);
			goto out;
		}
		migratetype = MIGRATE_MOVABLE;
	}
	pcp = &this_cpu_ptr(zone->pageset)->pcp;
	if(cold)
		list_add_tail(&page->lru,&pcp->lists[migratetype]);
	else
		list_add(&page->lru,&pcp->lists[migratetype]);
	pcp->count++;
	if(pcp->count >= pcp->high)
	{
		free_pcppages_bulk(zone,pcp->batch,pcp);
		pcp->count -= pcp->batch;
	}

out:
		;
//	local_irq_restore(flags);
}
static void __free_pages_ok(struct page *page,unsigned int order)
{
	unsigned long flags;
	int wasMlocked = __TestClearPageMlocked(page);

	if(!free_pages_prepare(page,order))
		return;

//	local_irq_save(flags);
	if(unlikely(wasMlocked))
		free_page_mlock(page);
//	__count_vm_evnets(PGFREE,1 << order);
	free_one_page(page_zone(page),page,order,
			get_pageblock_migratetype(page));
//	local_irq_restore(flags);

}
/*
 * __free_pages
 */
void __free_pages(struct page *page,unsigned long order)
{
	if(put_page_testzero(page))
	{
		if(order == 0)
			free_hot_cold_page(page,0);
		else
			__free_pages_ok(page,order);
	}
}
