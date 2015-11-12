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
	mm_debug("pfddd %p\n",pfn);
	pfn = pfn - zone->zone_start_pfn;
	return (pfn >> pageblock_order) * NR_PAGEBLOCK_BITS;
}
/*
 * Get the zone via page.
 */
static inline struct zone *page_zone(struct page *page)
{
	return &NODE_DATA(page_to_nid(page))->node_zones[page_zonenum(page)];
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

	mm_debug("start_bitidx %p\n",start_bitidx);
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
	mm_debug("321\n");
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
		mm_debug("2\n");
		INIT_LIST_HEAD(&page->lru);
		mm_debug("3\n");
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
	unsigned int addr = alloc_bootmem_core(pgdat,usemapsize);

	zone->pageblock_flags = NULL;

	mm_debug("pageblock_flags %p\n",addr);
	mm_debug("virt to phys %p\n",virt_to_phys((unsigned int)addr));
	mm_debug("phys to mem %p\n",phys_to_mem((unsigned int)virt_to_phys((unsigned int)addr)));
	if(usemapsize)
		/*
		 * In order to simulate physcail address and virtual address,we lead into
		 * virtual memory address.we can use virtual memory address directly.
		 */
		zone->pageblock_flags = 
			phys_to_mem(virt_to_phys(alloc_bootmem_core(pgdat,usemapsize)));
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
