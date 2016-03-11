#include "linux/kernel.h"
#include "linux/mmzone.h"
#include "linux/thread_info.h"
#include "linux/cpumask.h"
#include "linux/gfp.h"
#include "linux/cpuset.h"
#include "linux/kmemcheck.h"
#include "asm/current.h"
#include "linux/internal.h"
#include "linux/swap.h"
#include "linux/percpu.h"
#include "linux/page-flags.h"
#include "linux/mm_types.h"
#include "linux/nodemask.h"
#include "linux/ftree_event.h"
#include "linux/memory.h"
#include "linux/page.h"
#include "linux/mm.h"
#include "linux/bootmem.h"
#include "linux/vmstat.h"
#include "linux/printk.h"
#include "linux/hardirq.h"
#include "linux/memory_hotplug.h"
#include "linux/cpu.h"
#include "linux/debug_locks.h"
#include "linux/spinlock.h"
#include "linux/wait.h"
#include "linux/percpu-defs.h"
#include "linux/log2.h"
#include "asm/errno-base.h"
#include "linux/bounds.h"
#include "linux/mempolicy.h"
#include "linux/stop_machine.h"
#include "linux/topology.h"


/*
 * The ALLOC_WMARK bits are used as an index to zone->watermark.
 */
#define ALLOC_WMARK_MIN           WMARK_MIN
#define ALLOC_WMARK_LOW           WMARK_LOW
#define ALLOC_WMARK_HIGH          WMARK_HIGH
#define ALLOC_NO_WATERMARKS       0x04   /* don't check watermarks at all */

/* Mask to get the watermark bits */
#define ALLOC_WMARK_MASK          (ALLOC_NO_WATERMARKS - 1)

#define ALLOC_HARDER              0x10 /* try to alloc harder */
#define ALLOC_HIGH                0x20 /* __GFP_HIGH set */
#define ALLOC_CPUSET              0x40 /* check for correct cpuset */

/*
 * Boot pageset table.One per cpu which is going to be used for all
 * zones and all nodes.The parameters will be set in such a way
 * that an item put on a list will immediately be handed over to 
 * the buddy list.This is safe since pageset manipulation is done
 * with interrupts disabled.
 *
 * The boot_pagesets must be kept even after bootup is complete for
 * unused processors and/or zones.They do play a role for bootstrapping
 * hotplugged processor.
 *
 * zoneinfo_show() and maybe other function do not check if the
 * processor is online before following the pageset pointer.
 * Other parts of the kernel may not check if the zone is available.
 */
DEFINE_PER_CPU(struct per_cpu_pageset,boot_pageset);


/*
 * Memory init data
 */
static unsigned long __meminitdata dma_reserve;
static unsigned long __meminitdata nr_kernel_pages;
static unsigned long __meminitdata nr_all_pages;
gfp_t gfp_allowed_mask __read_mostly = GFP_BOOT_MASK;

/* movable_zone is the "real" zone pages in ZONE_MOVABLE are taken from */
int movable_zone;
int percpu_pagelist_fraction;

extern long vm_total_pages;
extern unsigned long highest_memmap_pfn;
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
 * Array of node states.
 */
nodemask_t node_states[NR_NODE_STATES] __read_mostly = {
//	[N_POSSIBLE] = NODE_MASK_ALL,
	// Need debug
	[N_POSSIBLE] = {{[0] = 1UL}},
	[N_ONLINE] = {{[0] = 1UL}},
#ifdef CONFIG_NUMA
	[N_NORMAL_MEMORY] = {{[0] = 1UL}},
#ifdef COFNIG_HIGHMEM
	[N_HIGH_MEMORY] = {{[0] = 1UL}},
#endif
	[N_CPU] = {{[0] = 1UL}},
#endif     /* NUMA */
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
static inline void cpuset_init_mems_allowed(void) {}
static int page_alloc_cpu_notify(void) {}
static inline void trace_mm_page_free_trace(struct page *p,int order) {}

/*
 * This array describes the order lists are fallen back to when
 * the free lists for the desirable migrate type are depleted.
 */
static int fallbacks[MIGRATE_TYPES][MIGRATE_TYPES - 1] = {
	[MIGRATE_UNMOVABLE] = {
		MIGRATE_RECLAIMABLE,MIGRATE_MOVABLE,MIGRATE_RESERVE
	},
	[MIGRATE_RECLAIMABLE] = {
		MIGRATE_UNMOVABLE,MIGRATE_MOVABLE,MIGRATE_RESERVE
	},
	[MIGRATE_MOVABLE] = {
		MIGRATE_RECLAIMABLE,MIGRATE_UNMOVABLE,MIGRATE_RESERVE
	},
	[MIGRATE_RESERVE] = {
		MIGRATE_RESERVE,MIGRATE_RESERVE,MIGRATE_RESERVE
	}, /* Never used */
};

/*
 * Helper functions to size the waitqueue hash table.
 * Essentially these want to choose hash table sizes sufficiently
 * large so that collisions trying to wait on pages are rare.
 * But in fact,the number of active page waitqueues on typical
 * systems is ridiculously low,less than 200.So this is even
 * conservative,even though it seems large.
 *
 * The constant PAGES_PER_WAITQUEUE specifies the ratio of pages to 
 * waitqueues,i.e.the size of the waitq table given the number of pages.
 */
#define PAGES_PER_WAITQUEUE      256

static inline unsigned long wait_table_hash_nr_entries(unsigned long pages)
{
	unsigned long size = 1;

	pages /= PAGES_PER_WAITQUEUE;

	while(size < pages)
		size <<= 1;

	/*
	 * Once we have dozens or even hundreds of threads sleeping
	 * on IO we've got bigger problems than wait queue collision.
	 * Limit the size of the wait table to a reasonable size.
	 */
	size = min(size,4096UL);

	return max(size,4UL);
}

/*
 * This is an integer logarithm so that shifts can be used later
 * to extract the more random high bits form the multiplicative
 * hash function before the remainder is taken.
 */
static inline unsigned long wait_table_bits(unsigned long size)
{
	return ffz(~size);
}

static int zone_wait_table_init(struct zone *zone,
		unsigned long zone_size_pages) 
{
	int i;
	struct pglist_data *pgdat = zone->zone_pgdat;
	size_t alloc_size;

	/*
	 * The per-page waitqueue mechanism uses hashed waitqueues
	 * per zone.
	 */
	zone->wait_table_hash_nr_entries = 
		wait_table_hash_nr_entries(zone_size_pages);
	zone->wait_table_bits = 
		wait_table_bits(zone->wait_table_hash_nr_entries);
	alloc_size = zone->wait_table_hash_nr_entries
					* sizeof(wait_queue_head_t);

	if(!slab_is_available()) {
		zone->wait_table = (wait_queue_head_t *)(unsigned long)
			(phys_to_mem(__pa(alloc_bootmem_node(pgdat,alloc_size))));
	} else {
		/*
		 * This case means that a zone whose size was 0 gets new memory
		 * via memory hot-add.
		 * But it may be the case that a new node was hot-added.In
		 * this case vmalloc() will not be able to use this new node's
		 * memory - this wait_table must be initialized to use this new 
		 * node itself as well.
		 * To use this new node's memory,further consideration will be
		 * necessary.
		 */
		zone->wait_table = 
			(wait_queue_head_t *)(unsigned long)vmalloc(alloc_size);
	}
	if(!zone->wait_table)
		return -ENOMEM;

	for(i = 0 ; i < zone->wait_table_hash_nr_entries ; ++i)
		init_waitqueue_head(zone->wait_table + i);

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
 * set_pageblock_flags_group - Set the requested group of flags for a 
 * @page: The page within the block of interest.
 * @start_bitidx: The first bit of interest.
 * @end_bitidx: The last bit of interest.
 * @flags:The flags to set.
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

	for(; start_bitidx <= end_bitidx ; start_bitidx++ , value <<= 1) {
		if(flags & value) {
			set_bit(bitidx + start_bitidx,bitmap);
		}
		else {
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
	for(pfn = start_pfn ; pfn < end_pfn ; pfn++) {
		/*
		 * There can be holes in boot-time mem_map[]s
		 * handed to this function.They do not exist 
		 * on hotplugged memory.
		 */
		if(context == MEMMAP_EARLY) {
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
#ifdef WANT_PAGE_VIRTUAL
		/* The shift won't overflow because ZONE_NORMAL is below 4G */
		if(!is_highmem_idx(zone))
			set_page_address(page,__va(pfn << PAGE_SHIFT));

#endif
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

static int zone_batchsize(struct zone *zone)
{
#ifdef CONFIG_MMU
	int batch;

	/*
	 * The per-cpu-page pools are set to around 1000th of the 
	 * size of the zone.But no more than 1/2 of a meg.
	 *
	 * Ok,so we don't know how big the cache is.So guess.
	 */
	batch = zone->present_pages / 1024;
	if(batch * PAGE_SIZE > 512 * 1024)
		batch = (512 * 1024) / PAGE_SIZE;
	batch /= 4;   /* We effectively *= 4 below */
	if(batch < 1)
		batch = 1;

	/*
	 * Clamp the batch to a 2^n - 1 value.Having a power
	 * of 2 value was found to be more likely to have
	 * suboptimal cache aliasing properties in some cases.
	 *
	 * For example if 2 tasks are alternately allocating
	 * batches of pages,one task can end up with a lot
	 * of pages of one half ot the possible page colors
	 * and the other with pages of the other colors.
	 */
	batch = rounddown_pow_of_two(batch + batch / 2) - 1;

	return batch;

#else
	/*
	 * The deferral and batching of frees should be suppressed under NOMMU
	 * conditions.
	 * 
	 * The problem is that NOMMU needs to be able to allocate large chunks
	 * of contiguous memory as there's no hardware page translation to 
	 * assemble apparent contiguous memory from discontiguous pages.
	 *
	 * Queueing large contiguous runs of pages for batching,however,
	 * causes the pages to actually be freed in smaller chunks.As there
	 * can be a significant delay between the individual batches being
	 * recycled,this leads to the once large chunks of space being
	 * fragmented and becoming unavailable for high-order allocatation.
	 */
	return 0;
#endif
}

static __meminit void zone_pcp_init(struct zone *zone)
{
	/*
	 * per cpu subsystem is not up at this point.The following code
	 * relies on the ability of the linker to provide the 
	 * offset of a per cpu variable into the per cpu area.
	 */
	zone->pageset = &boot_pageset;

	if(zone->present_pages)
		mm_debug("%s zone:%p pages,LIFO batch:%p\n",
				zone->name,(void *)(zone->present_pages),
				(void *)(unsigned long)zone_batchsize(zone));

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

	for_each_migratetype_order(order,t) {
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

	mm_debug("memmap_init initialising map node %d zone %lu"
			" pfns %p -> %p\n",
			pgdat->node_id,
			(unsigned long)zone_idx(zone),
			(void *)(unsigned long)zone_start_pfn,
			(void *)(unsigned long)(zone_start_pfn + size));

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
	usemapsize = roundup(usemapsize,8 * sizeof(unsigned int));

	return usemapsize / 8;
}

static void __init setup_usemap(struct pglist_data *pgdat,
		struct zone *zone,unsigned long zonesize)
{
	unsigned long usemapsize = usemap_size(zonesize);

	zone->pageblock_flags = NULL;
	if(usemapsize)
		/*
		 * In order to simulate physcail address and virtual address,
		 * we lead into virtual memory address.we can use virtual memory 
		 * address directly.
		 */
		zone->pageblock_flags = 
			(unsigned long *)(unsigned long)phys_to_mem(
					__pa(alloc_bootmem_node(pgdat,usemapsize)));

}
extern void __meminit pgdat_page_cgroup_init(struct pglist_data *pgdat);
/*
 * Set up the zone data struct.
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
	init_waitqueue_head(&pgdat->kswapd_wait);
	pgdat->kswapd_max_order = 0;
	pgdat_page_cgroup_init(pgdat);
	
	for(j = 0 ; j < MAX_NR_ZONES ; j++) {
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
		if(realsize >= memmap_pages) {
			realsize -= memmap_pages;
			if(memmap_pages)
				mm_debug("%s zone: %lu pages used for memmap\n",
						zone_names[j],memmap_pages);
		} else
			mm_err("%s zone:%lu pages exceeds realsize %lu\n",
					zone_names[j],memmap_pages , realsize);
		
		/* Account for reserved pages */
		if(j == 0 && realsize > dma_reserve) {
			realsize -= dma_reserve;
			mm_debug("%s zone: %lu pages reserved\n",
					zone_names[0],dma_reserve);
		}

		if(!is_highmem_idx(j))
			nr_kernel_pages += realsize;
		nr_all_pages += realsize;
		
		zone->spanned_pages = size;
		zone->present_pages = realsize;
		zone->name = zone_names[j];
		spin_lock_init(&zone->lock); 
		spin_lock_init(&zone->lru_lock);
		zone_seqlock_init(zone);
		zone->zone_pgdat = pgdat;
		
		zone_pcp_init(zone);
		for_each_lru(l) {
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
		BUG_ON(ret);
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

inline int populated_zone(struct zone *zone)
{
	return (!!zone->present_pages);
}

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

	BUG_ON(zone_type >= MAX_NR_ZONES);
	zone_type++;

	do {
		zone_type--;
		zone = pgdat->node_zones + zone_type;
		if(populated_zone(zone)) {
			zoneref_set_zone(zone,
					&zonelist->_zonerefs[nr_zones++]);
			check_highest_zone(zone_type);
		}

	} while(zone_type);
	return nr_zones;
}

#ifdef CONFIG_NUMA


#else

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
	for(node = local_node + 1 ; node < MAX_NUMNODES ; node++) {
		if(!node_online(node))
			continue;
		j = build_zonelists_node(NODE_DATA(node),zonelist,j,
				MAX_NR_ZONES - 1);
	}
	for(node = 0 ; node < local_node ; node++) {
		if(!node_online(node))
			continue;
		j = build_zonelists_node(NODE_DATA(node),zonelist,j,MAX_NR_ZONES - 1);
	}

	zonelist->_zonerefs[j].zone = NULL;
	zonelist->_zonerefs[j].zone_idx = 0;
}
#endif
/*
 * non-NUMA variant of zonelist performance cache - just NULL zlcache_ptr.
 */
static void build_zonelist_cache(struct pglist_data *pgdat)
{
	pgdat->node_zonelists[0].zlcache_ptr = NULL;
}
/*
 * Get the number of free pages of zone.
 */
static unsigned int nr_free_zone_pages(int offset)
{
	struct zoneref *z;
	struct zone *zone;

	/* Just pick one node,since fallback list is circular. */
	unsigned int sum = 0;

	struct zonelist *zonelist = node_zonelist(numa_node_id(),GFP_KERNEL);
	
	for_each_zone_zonelist(zone,z,zonelist,offset) {
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

static void setup_pageset(struct per_cpu_pageset *p,unsigned long batch)
{
	struct per_cpu_pages *pcp;
	int migratetype;

	memset(p,0,sizeof(*p));

	pcp = &p->pcp;
	pcp->count = 0;
	pcp->high = 6 * batch;
	pcp->batch = max(1UL, 1 * batch);
	for(migratetype = 0 ; migratetype < MIGRATE_PCPTYPES ; migratetype++)
		INIT_LIST_HEAD(&pcp->lists[migratetype]);
}

/* return values in  ...just for stop_machine() */
static __init_refok int __build_all_zonelists(void *data)
{
	int nid;
	int cpu;

	for_each_online_node(nid) {	
		struct pglist_data *pgdat = NODE_DATA(nid);
		
		build_zonelists(pgdat);
		build_zonelist_cache(pgdat);
	}

	/*
	 * Initialize the boot_pagesets that are going to be used
	 * for bootstrapping processors.The real pagesets for
	 * each zone will be allocated later when the per cpu
	 * allocator is available.
	 * 
	 * boot_pagesets are used also for bootstrapping offline
	 * cpus if the system is already booted because the pagesets
	 * are needed to initialize allocators on a specific cpu too.
	 * F.e. the percpu allocator needs the page allocator which
	 * needs the percpu allocator in order to allocate its pagesets
	 * (a chicken-egg dilemma).
	 */
	for_each_possible_cpu(cpu) 
		setup_pageset(&per_cpu(boot_pageset,cpu),0);


	return 0;
}

/*
 * Called with zonelist_mutex held always
 * Unless system_state == SYSTEM_BOOTING.
 */
void build_all_zonelists(void *data)
{
	set_zonelist_order();

	if(system_state == SYSTEM_BOOTING) {
		__build_all_zonelists(NULL);
		mminit_verify_zonelist();
		cpuset_init_current_mems_allowed();
	} else {
		/* we have to stop all cpus to guarantee there is no user
		 * of zonelist.
		 */
		stop_machine(__build_all_zonelists,NULL,NULL);
		/* cpuset refresh routine should be here. */
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

	mm_debug("page flags %p(",(void *)flags);

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
		mm_debug("%s %p",delim,(void *)flags);
	mm_debug(")\n");
}
/*
 * Dump information of page.
 */
void dump_page(struct page *page)
{
	mm_debug("page %p count %p mapcount %p mapping %p index %p\n",
			(void *)page,(void *)(unsigned long)atomic_read(&page->_count),
			(void *)(unsigned long)page_mapcount(page),
			(void *)page->mapping,(void *)page->index);
	dump_page_flags(page->flags);
}


static void bad_page(struct page *page)
{
	static unsigned long resume;
	static unsigned long nr_shown;
	static unsigned long nr_unshown;

	/* Don't complain about poisoned pages */
	if(PageHWPoison(page)) {
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
void __free_pages(struct page *page,unsigned long order);

/*
 * Permit the bootmem allocator to evade page validation on high-order frees.
 */
void __meminit __free_pages_bootmem(struct page *page,unsigned int order)
{
	if(order == 0) {
		__ClearPageReserved(page);
		set_page_count(page,0);
		set_page_refcounted(page);
		__free_page(page);
	} else {
		int loop;

		prefetchw(page);
		for(loop = 0 ; loop < BITS_PER_LONG ; loop++) {
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
				(page->flags & PAGE_FLAGS_CHECK_AT_FREE))) {
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

	if(!PageHighMem(page)) {
		debug_check_no_locks_freed((void *)(unsigned long)page_address(page),
				PAGE_SIZE << order);
		debug_check_no_obj_freed((void *)(unsigned long)page_address(page),
				PAGE_SIZE << order);
	}
	arch_free_page(page,order);
	kernel_map_pages(page,1 << order , 0);

	return true;
}
/*
 * get_pageblock_flags_group - Return the requested group of flags for the
 * pageblock_nr_pages of pages.
 * @page: The page within the block of interest.
 * @start_bitidx: The first bit of interest to retrieve
 * @end_bitidx: The last bit of interest
 * returns pageblock_bits flags
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

#ifdef BUDDY_DEBUG_PAGE_ORDER
void set_page_order(struct page *page,int order)
#else
static inline void set_page_order(struct page *page,int order)
#endif
{
	set_page_private(page,order);
	__SetPageBuddy(page);
}

#ifdef BUDDY_DEBUG_PAGE_ORDER
void rmv_page_order(struct page *page)
#else
static inline void rmv_page_order(struct page *page)
#endif
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
	if(PageBuddy(buddy) && page_order(buddy) == order) {
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

	/** 
	 * Need more debug..
	 * If the pfn 0x34000 and 0x65000 have same page_idx,
	 * kernel will deal with buddy_idx?
	 */
	page_idx = page_to_pfn(page) & ((1 << MAX_ORDER) - 1);

	VM_BUG_ON(page_idx & (( 1 << order) - 1));
	VM_BUG_ON(bad_range(zone,page));

	while(order < MAX_ORDER - 1) {
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
	if((order < MAX_ORDER - 2) && pfn_valid_within(page_to_pfn(buddy))) {
		struct page *higher_page,*higher_buddy;
		combined_idx = buddy_idx & page_idx;
		higher_page = page + (combined_idx - page_idx);
		buddy_idx = __find_buddy_index(combined_idx,order + 1);
		higher_buddy = page + (buddy_idx - combined_idx);
		if(page_is_buddy(higher_page,higher_buddy,order + 1)) {
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
	spin_lock(&zone->lock);
	zone->all_unreclaimable = 0;
	zone->pages_scanned = 0;

	__free_one_page(page,zone,order,migratetype);
	__mod_zone_page_state(zone,NR_FREE_PAGES,1 << order);
	spin_unlock(&zone->lock);
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

	spin_lock(&zone->lock);
	zone->all_unreclaimable = 0;
	zone->pages_scanned = 0;

	while(to_free) {
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
	spin_unlock(&zone->lock);
}

/*
 * Free a 0-order page
 * cold == 1 ? free cold page: free a hot page
 */
void free_hot_cold_page(struct page *page,int cold)
{
	struct zone *zone = page_zone(page);
	struct per_cpu_pages *pcp;
	unsigned long flags;
	int migratetype;
	int wasMlocked = __TestClearPageMlocked(page);


	if(!free_pages_prepare(page,0))
		return;

	migratetype = get_pageblock_migratetype(page);
	set_page_private(page,migratetype);
	local_irq_save(flags);
	if(unlikely(wasMlocked)) 
		free_page_mlock(page);
	
	/*
	 * We only track unmovable,reclaimable and movable on pcp lists.
	 * Free ISOLATE pages back to the allocator because they are being
	 * offlined but treat RESERVE as movable pages so we can get those
	 * areas back if necessary.Otherwise,we may have to free 
	 * excessively into the page allocator.
	 */
	if(migratetype >= MIGRATE_PCPTYPES) {
		if(unlikely(migratetype == MIGRATE_ISOLATE)) {
			free_one_page(zone,page,0,migratetype);
			goto out;
		}
		migratetype = MIGRATE_MOVABLE;
	}

	/**
	 * Need more debug ... pcp->high ?= 0,where do I set the value 
	 * of pcp->high? 
	 */
	pcp = &this_cpu_ptr(zone->pageset)->pcp;
	if(cold)
		list_add_tail(&page->lru,&pcp->lists[migratetype]);
	else
		list_add(&page->lru,&pcp->lists[migratetype]);
	pcp->count++;
	if(pcp->count >= pcp->high) {
		free_pcppages_bulk(zone,pcp->batch,pcp);
		pcp->count -= pcp->batch;
	}

out:
	local_irq_restore(flags);
}
static void __free_pages_ok(struct page *page,unsigned int order)
{
	unsigned long flags;
	int wasMlocked = __TestClearPageMlocked(page);

	if(!free_pages_prepare(page,order))
		return;

	local_irq_save(flags);
	if(unlikely(wasMlocked))
		free_page_mlock(page);

	free_one_page(page_zone(page),page,order,
			get_pageblock_migratetype(page));
	local_irq_restore(flags);

}

void __free_pages(struct page *page,unsigned long order)
{
	if(put_page_testzero(page)) {
		if(order == 0)
			free_hot_cold_page(page,0);
		else 
			__free_pages_ok(page,order);
	}
}
static inline int should_fail_alloc_page(gfp_t gfp_mask,unsigned int order)
{
	return 0;
}
static int zlc_zone_worth_trying(struct zonelist *zonelist,struct zoneref *z,
		nodemask_t *allowednodes)
{
	return 1;
}
/*
 * Return true if free pages are above 'mark'.This takes into account the order
 * of the allocation.
 */
static bool __zone_watermark_ok(struct zone *z,int order,unsigned long mark,
		int classzone_idx,int alloc_flags,long free_pages)
{
	/* free_page my go negative - that's OK */
	long min = mark;
	int o;

	free_pages -= (1 << order) + 1;
	if(alloc_flags & ALLOC_HIGH)
		min -= min / 2;
	if(alloc_flags & ALLOC_HARDER)
		min -= min / 4;

	if(free_pages <= min + z->lowmem_reserve[classzone_idx])
		return false;

	for(o = 0 ; o < order ; o++) {
		/* At the next order,this order's pages become unavailable */
		free_pages -= z->free_area[o].nr_free << o;

		/* Require fewer higher order pages to be free */
		/**
		 * We can set right shift is min_free_order_shift,set default as 4.
		 * Need more debug... Why does min right shift?
		 **/
		min >>= 1;

		if(free_pages <= min)
			return false;
	}
	return true;
}
bool zone_watermark_ok(struct zone *z,int order,unsigned long mark,
		int classzone_idx,int alloc_flags)
{
	return __zone_watermark_ok(z,order,mark,classzone_idx,alloc_flags,
			zone_page_state(z,NR_FREE_PAGES));
}


/*
 * The order of subdivision here is critical for the IO subsystem.
 * Pls don't alter this order without good reasons and regression
 * testing.Specifically,as large blocks of memory are subdivided,
 * the order in which smaller block are deliviered depends on the 
 * order they're subdibided in this function.This is the primary factor
 * influencing the order in which pages are delivered to the IO
 * subsytem according to emprirical testing,and this is also justified
 * by considering the behavior of a buddy system containing a single
 * large block of memory acted on by a seres of small allocations.
 * this behavior is a critical factor in sglist merging's success.
 */
static inline void expand(struct zone *zone,struct page *page,
		int low,int high,struct free_area *area,
		int migratetype)
{
	unsigned long size = 1 << high;

	while(high > low) {
		area--;
		high--;
		size >>= 1;
		VM_BUG_ON(bad_range(zone,&page[size]));
		list_add(&page[size].lru,&area->free_list[migratetype]);
		area->nr_free++;
		set_page_order(&page[size],high);
	}
}

/*
 * Go through the free lists for the given migratetype and remove
 * the smallest available page from the freelists.
 */
static inline struct page *__rmqueue_smallest(struct zone *zone,
		unsigned int order,int migratetype)
{
	unsigned int current_order;
	struct free_area *area;
	struct page *page;

	/* Find a page of the approproate size in preferred list */
	for(current_order = order ; current_order < MAX_ORDER ; ++current_order) {
		area = &(zone->free_area[current_order]);
		if(list_empty(&area->free_list[migratetype]))
			continue;

		page = list_entry(area->free_list[migratetype].next,
				struct page,lru);
		list_del(&page->lru);
		rmv_page_order(page);
		area->nr_free--;
		expand(zone,page,order,current_order,area,migratetype);
		return page;
	}
	return NULL;
}

/*
 * Move the free pages in a range to the free lists of the requested type.
 * Note that start_page and end_pages are not aligned on a pageblock
 * boundary.If alignment is required,use move_freepages_block()
 */
static int move_freepages(struct zone *zone,
		struct page *start_page,struct page *end_page,
		int migratetype)
{
	struct page *page;
	unsigned long order;
	int pages_moved = 0;

#ifndef CONFIG_HOLES_IN_ZONES
	/*
	 * page_zone is not safe to calii in this context when
	 * CONFIG_HOLES_IN_ZONE is set.This bug check is probably redundant
	 * anyway as we check zone boundaries in move_freepages_block().
	 * Remove at a later data when no bug reports exist related to 
	 * grouping pages by mobility.
	 */
	BUG_ON(page_zone(start_page) != page_zone(end_page));
#endif

	for(page = start_page ; page <= end_page ; ) {
		/* Make sure we are not inadvertently changing nodes */
		VM_BUG_ON(page_to_nid(page) != zone_to_nid(zone));
		
		if(!pfn_valid_within(page_to_pfn(page))) {
			page++;
			continue;
		}
	
		if(!PageBuddy(page)) {
			page++;
			continue;
		}
	
		order = page_order(page);
		list_del(&page->lru);
		list_add(&page->lru,
				&zone->free_area[order].free_list[migratetype]);
		page += 1 << order;
		pages_moved += 1 << order;
	}

	return pages_moved;
}

static int move_freepages_block(struct zone *zone,struct page *page,
		int migratetype)
{
	unsigned long start_pfn,end_pfn;
	struct page *start_page,*end_page;

	start_pfn  = page_to_pfn(page);
	start_pfn  = start_pfn & ~(pageblock_nr_pages - 1);
	start_page = pfn_to_page(start_pfn);
	end_page   = start_page + pageblock_nr_pages - 1;
	end_pfn    = start_pfn  + pageblock_nr_pages - 1;

	/* Do not cross boundaries */
	if(start_pfn < zone->zone_start_pfn)
		start_page = page;
	if(end_pfn >= zone->zone_start_pfn + zone->spanned_pages)
		return 0;

	return move_freepages(zone,start_page,end_page,migratetype);
}

static void change_pageblock_range(struct page *pageblock_page,
		int start_order,int migratetype)
{
	int nr_pageblock = 1 << (start_order - pageblock_order);

	while(nr_pageblock--) {
		set_pageblock_migratetype(pageblock_page,migratetype);
		pageblock_page += pageblock_nr_pages;
	}
}

/*
 * Remove an element from the buddy allocateor from the fallback list.
 */
static inline struct page *__rmqueue_fallback(struct zone *zone,
		int order,int start_migratetype)
{
	struct free_area *area;
	int current_order;
	struct page *page;
	int migratetype,i;

	/* Find the largest possible block of pages in the other list */
	for(current_order = MAX_ORDER - 1;current_order >= order; 
			--current_order) {
		for(i = 0 ; i < MIGRATE_TYPES - 1 ; i++) {
			migratetype = fallbacks[start_migratetype][i];

			/* MIGRATE_RESERVE handled later if necessary */
			if(migratetype == MIGRATE_RESERVE)
				continue;

			area = &(zone->free_area[current_order]);
			if(list_empty(&area->free_list[migratetype]))
				continue;
			page = list_entry(area->free_list[migratetype].next,
					struct page,lru);
			area->nr_free--;
			
			/*
			 * If breaking a large block of pages,move all free
			 * pages to the preferred allocation list.If falling
			 * back for a reclaimable kernel allocation,be more
			 * agressive about taking ownership of free pages.
			 */
			if(unlikely(current_order >= (pageblock_order >> 1)) ||
					start_migratetype == MIGRATE_RECLAIMABLE ||
					page_group_by_mobility_disabled) {
				unsigned long pages;
		
				pages = move_freepages_block(zone,page,
						start_migratetype);

				/* Claim the whole block if over half of it is free */
				if(pages >= (1 << (pageblock_order -1)) ||
						page_group_by_mobility_disabled) 
					set_pageblock_migratetype(page,start_migratetype);

				migratetype = start_migratetype;
			}

			/* Remove the page from the freelists */
			list_del(&page->lru);
			rmv_page_order(page);

			/* Take ownership for orders >= pageblock_order */
			if(current_order >= pageblock_order) 
				change_pageblock_range(page,current_order,
						start_migratetype);

			expand(zone,page,order,current_order,area,migratetype);
			
			return page;
		}
	}
	return NULL;
}

/*
 * Do the hard work of removing an element from the buddy allocator
 * Call me with the zone->lock already held.
 */
static struct page *__rmqueue(struct zone *zone,unsigned int order,
		int migratetype)
{
	struct page *page;

retry_reserve:
	page = __rmqueue_smallest(zone,order,migratetype);

	if(unlikely(!page) && migratetype != MIGRATE_RESERVE) {
		page = __rmqueue_fallback(zone,order,migratetype);

		/*
		 * Use MIGRATE_RESERVE rather than fail an allocation.goto
		 * is used because __rmqueue_smallest is an inline function
		 * and we want just one call site.
		 */
		if(!page) {
			migratetype = MIGRATE_RESERVE;
			goto retry_reserve;
		}
	}
	
	return page;
}

/*
 * Obtain a specified number of elements from the buddy allocator,all under 
 * a single hold of the lock,for efficiency.Add them to the supplied list
 * Returns the number of new pages which were placed at *list.
 */
static int rmqueue_bulk(struct zone *zone,unsigned int order,
		unsigned long count,struct list_head *list,
		int migratetype,int cold)
{
	int i;

	spin_lock(&zone->lock);
	for(i = 0 ; i < count ; i++) {
		struct page *page = __rmqueue(zone,order,migratetype);
		if(unlikely(page == NULL))
			break;

		/*
		 * Split buddy pages returned by expand() are received here
		 * in physical page order.The page is added to the callers and
		 * list and the list head then moves forward.From the callers
		 * perspective,the linked list is ordered by page number in 
		 * some conditions.This is useful for IO devices that can
		 * merge IO requests if the physical pages are ordered properly.
		 */
		if(likely(cold == 0))
			list_add(&page->lru,list);
		else
			list_add_tail(&page->lru,list);
		set_page_private(page,migratetype);
		list = &page->lru;
	}
	__mod_zone_page_state(zone,NR_FREE_PAGES, -(i << order));
	spin_unlock(&zone->lock);
	return i;
}

/*
 * This page is about to be retured from the page allocator.
 */
static inline int check_new_page(struct page *page)
{
	if(unlikely(page_mapcount(page) |
				(page->mapping != NULL) |
				(atomic_read(&page->_count) != 0) |
			    (page->flags & PAGE_FLAGS_CHECK_AT_PREP))) {
		return 1;
	}
	return 0;
}
static inline void prep_zero_page(struct page *page,int order,
		gfp_t gfp_flags)
{
	int i;

	/*
	 * Clear_highpage() will use KM_USER0,so it's a bug to use
	 * __GFP_ZERO and __GFP_HIGHMEM from hard or soft interrupt context.
	 */
	VM_BUG_ON((gfp_flags & __GFP_HIGHMEM) && in_interrupt());
	for(i = 0 ; i < ( 1 << order) ; i++)
		clear_highpage(page + i);
}
/*
 * Higher-order pages are called "compound pages". They are structured thusly.
 */
static void free_compound_page(struct page *page)
{
	__free_pages_ok(page,compound_order(page));
}
void prep_compound_page(struct page *page,unsigned long order)
{
	int i;
	int nr_pages = 1 << order;

	set_compound_page_dtor(page,free_compound_page);
	//set_compound_order(page,order);
	//__SetPageHead(page);
	for(i = 1 ; i < nr_pages ; i++)
	{
		struct page *p = page + i;
		/* Need debug */
		//__setPageTail(p);
		p->first_page = page;
	}
}

static int prep_new_page(struct page *page,int order,gfp_t gfp_flags)
{
	int i;

	for(i = 0 ; i < (1 << order) ; i++) {
		struct page *p = page + i;
		if(unlikely(check_new_page(p)))
			return 1;
	}

	set_page_private(page,0);
	set_page_refcounted(page);

	arch_alloc_page(page,order);
	kernel_map_pages(page, 1 << order , 1);

	if(gfp_flags & __GFP_ZERO)
		prep_zero_page(page,order,gfp_flags);

	/* Need more debug */
	if(order && (gfp_flags & __GFP_COMP))
		prep_compound_page(page,order);

	return 0;
}

/*
 * Really,prep_compound_page() should be called from __rmqueue_bulk().But 
 * we cheat by calling it from here.int the order > 0 path.Saves a branch
 * or two.
 */
static inline struct page *buffered_rmqueue(struct zone *preferred_zone,
		struct zone *zone,int order,gfp_t gfp_flags,
		int migratetype)
{
	unsigned long flags;
	struct page *page;
	int cold = !!(gfp_flags & __GFP_COLD);

again:
	if(likely(order == 0)) {
		struct per_cpu_pages *pcp;
		struct list_head *list;

		local_irq_save(flags);
		pcp = &this_cpu_ptr(zone->pageset)->pcp;
		list = &pcp->lists[migratetype];
		if(list_empty(list)) {
			pcp->count += rmqueue_bulk(zone,0,
					pcp->batch,list,
					migratetype,cold);
			if(unlikely(list_empty(list)))
				goto failed;
		}
	
		if(cold)
			page = list_entry(list->prev,struct page,lru);
		else
			page = list_entry(list->next,struct page,lru);
		list_del(&page->lru);
		pcp->count--;
	} else {
		if(unlikely(gfp_flags & __GFP_NOFAIL)) {
			/*
			 * __GFP_NOFAIL is not to be used in new code.
			 *
			 * All __GFP_NOFAIIL callers should be fixed so that they
			 * properly detect and handle allocation failures.
			 *
			 * We most definitely don't want callers attempting to
			 * allocate greater than order - 1 page units with
			 * __GFP_NOFAIL.
			 */
			WARN_ON_ONCE(order - 1);
		}
		spin_lock_irqsave(&zone->lock,flags);
		page = __rmqueue(zone,order,migratetype);
		spin_unlock(&zone->lock);
		if(!page)
			goto failed;
		__mod_zone_page_state(zone,NR_FREE_PAGES,-(1 << order));
	}

	zone_statistics(preferred_zone,zone);
	local_irq_restore(flags);

	VM_BUG_ON(bad_range(zone,page));
	if(prep_new_page(page,order,gfp_flags))
		goto again;
	
	return page;

failed:
	local_irq_restore(flags);
	return NULL;
}
static void zlc_mark_zone_full(struct zonelist *zonelist,struct zoneref *z)
{
}
static nodemask_t *zlc_setup(struct zonelist *zonelist,int alloc_flags)
{
	return NULL;
}

/*
 * get_page_from_freelist goes through the zonelist trying to allocate
 * a page.
 */
static struct page *get_page_from_freelist(gfp_t gfp_mask,
		nodemask_t *nodemask,unsigned int order,struct zonelist *zonelist,
		int high_zoneidx,int alloc_flags,
		struct zone *preferred_zone,int migratetype)
{
	struct zoneref *z;
	struct page *page = NULL;
	int classzone_idx;
	struct zone *zone;
	nodemask_t *allowednodes = NULL; /* zonelist_cache approximation */
	int zlc_active = 0;     /* set if using zonelist_cache */
	int did_zlc_setup = 0;    /* just call zlc_setup() one time */

	classzone_idx = zone_idx(preferred_zone);
zonelist_scan:
	/*
	 * Scan zonelist,looking for a zone with enough free.
	 * See also cpuset_zone_allowed() comment in kernel/cpuset.c
	 */
	for_each_zone_zonelist_nodemask(zone,z,zonelist,
					high_zoneidx,nodemask) {
		if(NUMA_BUILD && zlc_active &&
				!zlc_zone_worth_trying(zonelist,z,allowednodes))
			continue;
		if((alloc_flags & ALLOC_CPUSET) &&
				!cpuset_zone_allowed_softwall(zone,gfp_mask))
			goto try_next_zone;

		BUILD_BUG_ON(ALLOC_NO_WATERMARKS < NR_WMARK);
		if(!(alloc_flags & ALLOC_NO_WATERMARKS)) {
			unsigned long mark;
			int ret;

			/**
			 * Need more debug.Where does kernel set watermark.
			 **/
			mark = zone->watermark[alloc_flags & ALLOC_WMARK_MASK];
			if(zone_watermark_ok(zone,order,mark,
						classzone_idx,alloc_flags))
				goto try_this_zone;

			if(zone_reclaim_mode == 0)
				goto this_zone_full;

			ret = zone_reclaim(zone,gfp_mask,order);
			switch(ret)
			{
				case ZONE_RECLAIM_NOSCAN:
					/* did not scan */
					goto try_next_zone;
				case ZONE_RECLAIM_FULL:
					/* scanned but unreclaimable */
					goto this_zone_full;
				default:
					/* did we relcaim enough */
					if(!zone_watermark_ok(zone,order,mark,
								classzone_idx,alloc_flags))
						goto this_zone_full;
			}
		}

try_this_zone:
		page = buffered_rmqueue(preferred_zone,zone,order,
				gfp_mask,migratetype);
		if(page)
			break;
this_zone_full:
		if(NUMA_BUILD)
			zlc_mark_zone_full(zonelist,z);
try_next_zone:
		if(NUMA_BUILD && !did_zlc_setup && nr_online_nodes > 1) {
			/*
			 * We do zlc_setup after the first zone is tried but only
			 * if there are multiple nodes make it worthwhile
			 */
			allowednodes = zlc_setup(zonelist,alloc_flags);
			zlc_active = 1;
			did_zlc_setup = 1;
		}
	}
	if(unlikely(NUMA_BUILD && page == NULL && zlc_active)) {
		/* Disable zlc cache for second zonelist scan */
		zlc_active = 0;
		goto zonelist_scan;
	}
	return page;
}
static inline void wake_all_kswapd(unsigned int order,
		struct zonelist *zonelist,enum zone_type high_zoneidx,
		enum zone_type classzone_idx)
{
	struct zoneref *z;
	struct zone *zone;

	for_each_zone_zonelist(zone,z,zonelist,high_zoneidx)
//		wakeup_kswapd(zone,order,classzone_idx);
		;
}
static inline int gfp_to_alloc_flags(gfp_t gfp_mask)
{
	int alloc_flags = ALLOC_WMARK_MIN | ALLOC_CPUSET;
	const gfp_t wait = gfp_mask & __GFP_WAIT;

	/* __GFP_HIGH is assumed to be the same as ALLOC_HIGH to save a branch */
	BUILD_BUG_ON(__GFP_HIGH != (gfp_t)ALLOC_HIGH);

	/*
	 * The caller may dip into page reserves a bit more if the caller
	 * cannot run direct reclaim,or if the caller has realtime scheduling
	 * policy or is asking for __GFP_HIGH memory.GFP_ATOMIC requests will
	 * set both ALLOC_HARDER(!wait) and ALLOC_HIGH(__GFP_HIGH)
	 */
	alloc_flags |= (int)(gfp_mask & __GFP_HIGH);

	if(!wait)
	{
		/*
		 * Not worth trying to allocate harder for
		 * __GFP_NOMEMALLOC even if it can't schedule.
		 */
		if(!(gfp_mask & __GFP_NOMEMALLOC))
			alloc_flags |= ALLOC_HARDER;
		/*
		 * lgnore cpuset if GFP_ATOMIC(!wait) rether than fail alloc.
		 * See also cpuset_zone_allowed() comment in kernel/cpuset.c
		 */
		alloc_flags &= ~ALLOC_CPUSET;
	} else if(unlikely(rt_task(current)) && !in_interrupt())
		alloc_flags |= ALLOC_HARDER;
	
	if(likely(!(gfp_mask & __GFP_NOMEMALLOC)))
	{
		if(!in_interrupt() &&
				((current->flags & PF_MEMALLOC) ||
				unlikely(test_thread_flags(TIF_MEMDIE)))) 
			alloc_flags |= ALLOC_NO_WATERMARKS;
	}
	return alloc_flags;
}
/*
 * This is called in the allocator slow-path if the allocation request is of
 * sufficient urgency to ignore watermarks and take other desperate measures.
 */
static inline struct page *__alloc_pages_high_priority(
		gfp_t gfp_mask,unsigned int order,
		struct zonelist *zonelist,enum zone_type high_zoneidx,
		nodemask_t *nodemask,struct zone *preferred_zone,
		int migratetype)
{
	struct page *page;

	do {
		page = get_page_from_freelist(gfp_mask,nodemask,order,
				zonelist,high_zoneidx,ALLOC_NO_WATERMARKS,
				preferred_zone,migratetype);

		if(!page && gfp_mask & __GFP_NOFAIL)
//			wait_iff_congested(preferred_zone,BLK_RW_ASYNC,HZ / 50);
			;
	} while(!page && (gfp_mask & __GFP_NOFAIL));

	return page;
}
static inline struct page *__alloc_pages_direct_compact(gfp_t gfp_mask,
		unsigned int order,struct zonelist *zonelist,
		enum zone_type high_zoneidx,nodemask_t *nodemask,
		int alloc_flags,struct zone *preferred_zone,
		int migratetype,unsigned long *did_some_progress,
		bool sync_migration)
{
	return NULL;
}
static inline int should_alloc_retry(gfp_t gfp_mask,unsigned int order,
		unsigned long pages_reclaimed)
{
	/* Do not loop of specifically requested */
	if(gfp_mask & __GFP_NORETRY)
		return 0;
	
	/*
	 * In this implementation,order <= PAGE_ALLOC_COSTLY_ORDER
	 * means __GFP_NOFAIL,but that may not be true in other 
	 * implementations.
	 */
	if(order <= PAGE_ALLOC_COSTLY_ORDER)
		return 1;

	/*
	 * For order > PAGE_ALLOC_COSTLY_ORDER,if __GFP_REPEAT is 
	 * specified,then we retry until we no longer reclaim any pages.
	 * or we've reclaimed an order of pages at least as
	 * large as allocation's order.In both cases,if the
	 * allocation still,we stop retrying.
	 */
	if(gfp_mask & __GFP_REPEAT && pages_reclaimed < (1 << order))
		return 1;

	/*
	 * Don't let big-order allocations loop unless the caller
	 * explicitly requests that.
	 */
	if(gfp_mask & __GFP_NOFAIL)
		return 1;

	return 0;
}

static inline struct page *__alloc_pages_slowpath(gfp_t gfp_mask,
		unsigned int order,struct zonelist *zonelist,
		enum zone_type high_zoneidx,
		nodemask_t *nodemask,struct zone *preferred_zone,
		int migratetype)
{
	const gfp_t wait = gfp_mask & __GFP_WAIT;
	struct page *page = NULL;
	int alloc_flags;
	unsigned long pages_reclaimed = 0;
	unsigned long did_some_progress;
	bool sync_migration = false;

	/*
	 * In the slowpath,we sanity check order to avoid ever trying to 
	 * reclaim >= MAX_ORDER areas which will never succeed.Callers may
	 * be using allocators in order of preference for an area that is
	 * too large.
	 */
	if(order >= MAX_ORDER) {
		WARN_ON_ONCE(!(gfp_mask & __GFP_NOWARN));
		return NULL;
	}

	/*
	 * GFP_THISNODE (meaning __GFP_THISNODE,__GFP_NORETRY and
	 * __GFP_NOWARN set) should not cause reclaim since the subsystem
	 * unsing GFP_THISNODE may choose to trigger reclaim
	 * using a large set of nodes after it has established that the 
	 * allowed per node queues are empty and that nodes are
	 * over allocated.
	 */
	if(NUMA_BUILD && (gfp_mask & GFP_THISNODE) == GFP_THISNODE)
		goto nopage;

restart:
	if(!(gfp_mask & __GFP_NO_KSWAPD))
		 wake_all_kswapd(order,zonelist,high_zoneidx,
				zone_idx(preferred_zone));

	/*
	 * OK,we're below the kswapd watermark and have kicked backgroud
	 * reclaim.Now things get more complex,so set up alloc_flags according
	 * to how we want to proceed.
	 */
	alloc_flags = gfp_to_alloc_flags(gfp_mask);

	/*
	 * Find the true preferred zone if the allocation is unconstrained by
	 * cpusets.
	 */
	page = get_page_from_freelist(gfp_mask,nodemask,order,zonelist,
			high_zoneidx,alloc_flags & ~ALLOC_NO_WATERMARKS,
			preferred_zone,migratetype);

	if(page)
		goto got_pg;
	
rebalance:
	/* Allocate without watermarks if the context allows */
	if(alloc_flags & ALLOC_NO_WATERMARKS) {
		page = __alloc_pages_high_priority(gfp_mask,order,
				zonelist,high_zoneidx,nodemask,
				preferred_zone,migratetype);
		if(page)
			goto got_pg;
	}

	/* Atomic allocations - we can't balance anything */
	if(!wait)
		goto nopage;

	/* Avoid recursion of direct reclaim */
	if(current->flags & PF_MEMALLOC)
		goto nopage;

	/* Avoid allocations with no watermarks from looping endlessly */
	if(test_thread_flags(TIF_MEMDIE) && !(gfp_mask & __GFP_NOFAIL))
		goto nopage;

	/*
	 * Try direct compation.The first pass is asynchronous.Subsequent
	 * attempts aftect reclaim are synchronous.
	 */
	page = __alloc_pages_direct_compact(gfp_mask,order,
			zonelist,high_zoneidx,
			nodemask,
			
			alloc_flags,preferred_zone,
			migratetype,&did_some_progress,
			sync_migration);
	if(page)
		goto got_pg;
	sync_migration = true;
/*
 * No support reclaim.
 */
#if 0
	/* Try direct reclaim and then allocating */
	page = __alloc_pages_direct_reclaim(gfp_mask,order,
			zonelist,high_zoneidx,
			nodemask,
			alloc_flags,preferred_zone,
			migratetype,&did_some_progress);
	if(page)
		goto got_pg;
#endif
	/*
	 * If we failed to make any progree reclaiming,then we are
	 * running out of options and have to consider going OOM.
	 */
#if 0
	if(!did_some_progress)
	{
		if((gfp_mask & __GFP_FS) && !(gfp_mask & __GFP_NORETRY))
		{
			if(oom_killer_disabled)
				goto nopage;
			page = __alloc_pages_may_oom(gfp_mask,order,
					zonelist,high_zoneidx,
					nodemask,preferred_zone,
					migratetype);
			if(page)
				goto got_pg;

			if(!(gfp_mask & __GFP_NOFAIL))
			{
				/*
				 * The oom killer is not called for high-order
				 * allocations that may fail,so if no progress
				 * is being made,there are no other options and
				 * retrying is unlikely to help.
				 */
				if(order > PAGE_ALLOC_COSTLY_ORDER)
					goto nopage;
				/*
				 * The oom killer is not called for lowmem
				 * allocation to prevent needlessly killing
				 * innocent tasks.
				 */
				if(high_zoneidx < ZONE_NORMAL)
					goto nopage;
			}
			goto restart;
		}
	}
#endif
	/* Check if we should retry the allocation */
	pages_reclaimed += did_some_progress;
	if(should_alloc_retry(gfp_mask,order,pages_reclaimed))
	{
		/* Wait for some write requests to complete then retry */
//		wait_iff_congested(preferred_zone,BLK_RW_ASYNC,HZ/50);
		goto rebalance;
	} else
	{
		/*
		 * High-order allocation do not necessarily loop after
		 * direct reclaim and reclaim/compaction depends on compaction
		 * being called after reclaim so call directly if necessary
		 */
		page = __alloc_pages_direct_compact(gfp_mask,order,
				zonelist,high_zoneidx,
				nodemask,alloc_flags,preferred_zone,
				migratetype,&did_some_progress,
				sync_migration);
		if(page)
			goto got_pg;
	}
nopage:
	if(!(gfp_mask & __GFP_NOWARN) && printk_ratelimit()) {
		mm_debug("%s:page allocation failure."
				" order:%d,mode:0x%x\n",
				current->comm,order,gfp_mask);
		dump_stack();
		show_mem();
	}
	return page;
got_pg:
	if(kmemcheck_enabled)
		kmemcheck_pagealloc_alloc(page,order,gfp_mask);
	return page;
}

/*
 * This is the 'heart' of the zoned buddy allocator.
 */
struct page *__alloc_pages_nodemask(gfp_t gfp_mask,unsigned int order,
		struct zonelist *zonelist,nodemask_t *nodemask)
{
	enum zone_type high_zoneidx = gfp_zone(gfp_mask);
	struct zone *preferred_zone;
	struct page *page;
	int migratetype = allocflags_to_migratetype(gfp_mask);

	gfp_mask &= gfp_allowed_mask;

	lockdep_trace_alloc(gfp_mask);

	might_sleep_if(gfp_mask & __GFP_WAIT);

	if(should_fail_alloc_page(gfp_mask,order))
		return NULL;

	/*
	 * Check the zone suitable for the gfp_mask contain at least one 
	 * valid zone.It's possible to have an empty zonelist as a result
	 * of GFP_THISNODE and a memoryless node.
	 */
	if(unlikely(!zonelist->_zonerefs->zone))
		return NULL;

	get_mems_allowed();
	/* The preferred zone is used for statistics later. */
	first_zones_zonelist(zonelist,high_zoneidx,
			nodemask ? : &cpuset_current_mems_allowed,
			&preferred_zone);
	if(!preferred_zone) {
		put_mems_allowed();
		return NULL;
	}

	/*
	 * First allocation attempt.
	 */
	page = get_page_from_freelist(gfp_mask | __GFP_HARDWALL,
			nodemask,order,zonelist,high_zoneidx,
			ALLOC_WMARK_LOW | ALLOC_CPUSET,
			preferred_zone,migratetype);

	if(unlikely(!page))
		page = __alloc_pages_slowpath(gfp_mask,order,
				zonelist,high_zoneidx,nodemask,
				preferred_zone,migratetype);

	put_mems_allowed();

	return page;
}

/*
 * Common helper functions.
 */
unsigned long __get_free_pages(gfp_t gfp_mask,unsigned int order)
{
	struct page *page;

	/*
	 * __get_free_pages() returns a 32-bit address,which cannot represent
	 * a highmem page.
	 */
	VM_BUG_ON((gfp_mask & __GFP_HIGHMEM) != 0);

	page = alloc_pages(gfp_mask,order);
	if(!page)
		return 0;
	return (unsigned long)page_address(page);
}

static inline void show_node(struct zone *zone)
{
	if(NUMA_BUILD)
		mm_debug("Node %d\n",zone_to_nid(zone));
}

#define K(x)  ((x) << (PAGE_SHIFT - 10))
/*
 * Show free aera list(used inside shifi_scroll-lock stuff)
 * We also calculate the precentage fragmentation.We do this by conting the
 * memory on each free list with the exception of the first item on the list.
 */
void show_free_areas(void)
{
	int cpu;
	struct zone *zone;

	for_each_populated_zone(zone) {
		show_node(zone);
		mm_debug("%s per-cpu\n",zone->name);

		for_each_online_cpu(cpu) {
			struct per_cpu_pageset *pageset;

			pageset = per_cpu_ptr(zone->pageset,cpu);

			mm_debug("CPU %p:hi:%p,btch:%p usd:%p\n",
					(void *)(unsigned long)cpu,
					(void *)(unsigned long)pageset->pcp.high,
					(void *)(unsigned long)pageset->pcp.batch,
					(void *)(unsigned long)pageset->pcp.count);
		}
	}
	mm_debug("active_anon:%p\n"
			 "inactive_anon:%p\n"
			 "isolate_anon:%p\n"
			 "active_file:%p\n"
			 "inactive_file:%p\n"
			 "isolate_file:%p\n"
			 "unevictable:%p\n"
			 "dirty:%p\n"
			 "writeback:%p\n"
			 "unstable:%p\n"
			 "free:%p\n"
			 "slab_reclaimable:%p\n"
			 "slab_unreclaimable:%p\n"
			 "mapped:%p\n"
			 "shmem:%p\n"
			 "pagetables:%p\n"
			 "bounce:%p\n",
			 (void *)(unsigned long)global_page_state(NR_ACTIVE_ANON),
			 (void *)(unsigned long)global_page_state(NR_INACTIVE_ANON),
			 (void *)(unsigned long)global_page_state(NR_ISOLATED_ANON),
			 (void *)(unsigned long)global_page_state(NR_ACTIVE_FILE),
			 (void *)(unsigned long)global_page_state(NR_INACTIVE_FILE),
			 (void *)(unsigned long)global_page_state(NR_ISOLATED_FILE),
			 (void *)(unsigned long)global_page_state(NR_UNEVICTABLE),
			 (void *)(unsigned long)global_page_state(NR_FILE_DIRTY),
			 (void *)(unsigned long)global_page_state(NR_WRITEBACK),
			 (void *)(unsigned long)global_page_state(NR_UNSTABLE_NFS),
			 (void *)(unsigned long)global_page_state(NR_FREE_PAGES),
			 (void *)(unsigned long)global_page_state(NR_SLAB_RECLAIMABLE),
			 (void *)(unsigned long)global_page_state(NR_SLAB_UNRECLAIMABLE),
			 (void *)(unsigned long)global_page_state(NR_FILE_MAPPED),
			 (void *)(unsigned long)global_page_state(NR_SHMEM),
			 (void *)(unsigned long)global_page_state(NR_PAGETABLE),
			 (void *)(unsigned long)global_page_state(NR_BOUNCE));

	for_each_populated_zone(zone) {
		int i;

		show_node(zone);
		mm_debug("%s\n"
				"free:%pkB\n"
				"min:%pkB\n"
				"low:%pkB\n"
				"high:%pkB\n"
				"active_anon:%pkB\n"
				"inactive_anon:%pkB\n"
				"active_file:%pkB\n"
				"inactive_file:%pkB\n"
				"unevictable:%pkB\n"
				"isolated(anon):%pkB\n"
				"isolated(file):%pkB\n"
				"present:%pkB\n"
				"mlocked:%pkB\n"
				"dirty:%pkB\n"
				"writeback:%pkB\n"
				"mapped:%pkB\n"
				"shmem:%pkB\n"
				"slab_reclaimable:%pkB\n"
				"slab_unreclaimable:%pkB\n"
				"kernel_stack:%pkB\n"
				"pagetables:%pkB\n"
				"unstable:%pkB\n"
				"bounce:%pkB\n"
				"writeback_tmp:%pkB\n"
				"pages_scanned:%pkB\n"
				"all_unreclaimable?%s\n",
			zone->name,
			(void *)(unsigned long)K(zone_page_state(zone,NR_FREE_PAGES)),
			(void *)(unsigned long)K(min_wmark_pages(zone)),
			(void *)(unsigned long)K(low_wmark_pages(zone)),
			(void *)(unsigned long)K(high_wmark_pages(zone)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_ACTIVE_ANON)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_INACTIVE_ANON)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_ACTIVE_FILE)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_INACTIVE_FILE)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_UNEVICTABLE)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_ISOLATED_ANON)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_ISOLATED_FILE)),
			(void *)(unsigned long)K(zone->present_pages),
			(void *)(unsigned long)K(zone_page_state(zone,NR_MLOCK)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_FILE_DIRTY)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_WRITEBACK)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_FILE_MAPPED)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_SHMEM)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_SLAB_RECLAIMABLE)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_SLAB_UNRECLAIMABLE)),
			(void *)(unsigned long)(zone_page_state(zone,NR_KERNEL_STACK) *
				THREAD_SIZE / 1024),
			(void *)(unsigned long)K(zone_page_state(zone,NR_PAGETABLE)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_UNSTABLE_NFS)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_BOUNCE)),
			(void *)(unsigned long)K(zone_page_state(zone,NR_WRITEBACK_TEMP)),
			(void *)(unsigned long)zone->pages_scanned,
			(zone->all_unreclaimable ? "yes" : "no")
				);
		mm_debug("lowmem_reserve[]:");
		for(i = 0 ; i < MAX_NR_ZONES ; i++)
			mm_debug(" %p",(void *)zone->lowmem_reserve[i]);
		mm_debug("\n");
	}
	
	for_each_populated_zone(zone) {
		unsigned long nr[MAX_ORDER],flags,order,total = 0;

		show_node(zone);
		mm_debug("%s:",zone->name);

//		spin_lock_irqsave(&zone->lock,flags);
		for(order = 0 ; order < MAX_ORDER ; order++)
		{
			nr[order] = zone->free_area[order].nr_free;
			total += nr[order] << order;
		}
//		spin_unlock_irqrestore(&zone->lock,flags);
		for(order = 0 ; order < MAX_ORDER ; order++)
			mm_debug(" %p %pkB ",(void *)nr[order],(void *)(K(1UL) << order));
		mm_debug("= %pkB\n",(void *)K(total));
	}
	mm_debug("%p total pagecache pages\n",
			(void *)global_page_state(NR_FILE_PAGES));

	show_swap_cache_info();
}

void free_pages(unsigned long addr,unsigned int order)
{
	if(addr != 0) {
		VM_BUG_ON(!virt_addr_valid((void *)addr));
		__free_pages(virt_to_page((void *)addr),order);
	}
}



/*
 * Allocate node mem map.
 */
static __init void alloc_node_mem_map(struct pglist_data *pgdat)
{
	/* Skip empty nodes. */
	if(!pgdat->node_spanned_pages)
		return;

#ifdef CONFIG_FLAT_NODE_MEM_MAP
	/* ia64 gets its own node_mem_map,before this,without bootmem */
	if(!pgdat->node_mem_map) {
		unsigned long size,start,end;
		struct page *map;

		/*
		 * The zone's endpoints aren't required to be MAX_ORDER
		 * aligned but the node_mem_map endpoints must be in order
		 * for the buddy allocator to function correctly.
		 */
		start = pgdat->node_start_pfn & ~(MAX_ORDER_NR_PAGES - 1);
		end   = pgdat->node_start_pfn + pgdat->node_spanned_pages;
		end   = ALIGN(end,MAX_ORDER_NR_PAGES);
		size  = (end - start) * sizeof(struct page);
		map   = alloc_remap(pgdat->node_id,size);
		if(!map)
			map = (struct page *)(unsigned long)alloc_bootmem_node(pgdat,size);
		
		/*
		 * In order to use node_mem_map directly,we use virtual memory address 
		 * to replace the physcial address.Note,all address which allocate from
		 * virtual memory use virtual memory address.
		 */
		pgdat->node_mem_map = 
			(struct page *)(unsigned long)(phys_to_mem(__pa(map))) + 
				(pgdat->node_start_pfn - start);
	}
#ifndef CONFIG_NEED_MULTIPLE_NODES
	if(pgdat == NODE_DATA(0)) {
		mem_map = 
			(struct page *)(unsigned long)(
					mem_to_phys(NODE_DATA(0)->node_mem_map));
	}
#endif
#endif
}


/*
 * Calculate the pages of pglist_data
 */
static void calculate_node_totalpages(struct pglist_data *pgdat,
		unsigned long *zone_sizes,unsigned long *zhole_size)
{
	unsigned long realpages,totalpages = 0;
	enum zone_type i;

	for(i = 0 ; i < MAX_NR_ZONES ; i++) 
		totalpages += zone_spanned_pages_in_node(pgdat->node_id,
				i,zone_sizes);
	
	pgdat->node_spanned_pages = totalpages;

	realpages = totalpages;
	for(i = 0 ; i < MAX_NR_ZONES ; i++)
		realpages -= zone_absent_pages_in_node(pgdat->node_id,
				i,zhole_size);
	
	pgdat->node_present_pages = realpages;
	mm_debug("On node %d totalpages %p\n",pgdat->node_id,
			(void *)(unsigned long)realpages);
}

__init void free_area_init_node(int nid,unsigned long *zone_sizes,
		unsigned long start_pfn,unsigned long *zhole_size)
{
	pg_data_t *pgdat = NODE_DATA(nid);

	pgdat->node_id = nid;
	pgdat->node_start_pfn = start_pfn;
	calculate_node_totalpages(pgdat,zone_sizes,zhole_size);

	alloc_node_mem_map(pgdat);
#ifdef CONFIG_FLAT_NODE_MEM_MAP
	mm_debug("free_area_init_node:node %d,pgdat %p,node_mem_map %p\n",
			pgdat->node_id,(void *)pgdat,(void *)pgdat->node_mem_map);
#endif
	free_area_init_core(pgdat,zone_sizes,zhole_size);
}

/*
 * setup_pagelist_highmark() sets the high mark for hot per_cpu_pagelist
 * to the value high for the pageset p.
 */
static void setup_pagelist_highmark(struct per_cpu_pageset *p,
		unsigned long high)
{
	struct per_cpu_pages *pcp;

	pcp = &p->pcp;
	pcp->high = high;
	pcp->batch = max(1UL,high / 4);
	if((high / 4) > (PAGE_SHIFT * 8))
		pcp->batch = PAGE_SHIFT * 8;
}

static __meminit void setup_zone_pageset(struct zone *zone)
{
	int cpu;

	zone->pageset = alloc_percpu(struct per_cpu_pageset);

	for_each_possible_cpu(cpu) {
		struct per_cpu_pageset *pcp = per_cpu_ptr(zone->pageset,cpu);

		setup_pageset(pcp,zone_batchsize(zone));

		if(percpu_pagelist_fraction)
			setup_pagelist_highmark(pcp,
					(zone->present_pages /
					 percpu_pagelist_fraction));
	}
}

/*
 * Allocate per cpu pagesets and initialize them.
 * Before this call only boot pagesets were available.
 */
void __init setup_per_cpu_pageset(void)
{
	struct zone *zone;

	for_each_populated_zone(zone)
		setup_zone_pageset(zone);
}

/*
 * Amount of free RAM allocatable within ZONE_DMA and ZONE_NORMAL
 */
unsigned int nr_free_buffer_pages(void)
{
	return nr_free_zone_pages(gfp_zone(GFP_USER));
}
