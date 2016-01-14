#ifndef _MMZONE_H_
#define _MMZONE_H_

#include "bitops.h"
#include "highmem.h"
#include "pageblock-flags.h"
#include "list.h"
#include "page_cgroup.h"
#include "nodemask.h"
#include "spinlock_types.h"
#include "wait.h"

enum zone_type {

#ifdef CONFIG_ZONE_DMA
	ZONE_DMA,
#endif
#ifdef CONFIG_ZONE_DMA32
	ZONE_DMA32,
#endif
	ZONE_NORMAL,
#ifdef CONFIG_HIGHMEM
	ZONE_HIGHMEM,
#endif
	ZONE_MOVABLE,
	__MAX_NR_ZONES
};

enum memmap_context {
	MEMMAP_EARLY,
	MEMMAP_HOTPLUG,
};
enum zone_watermarks {
	WMARK_MIN,
	WMARK_LOW,
	WMARK_HIGH,
	NR_WMARK
};

#define min_wmark_pages(z) (z->watermark[WMARK_MIN])
#define low_wmark_pages(z) (z->watermark[WMARK_LOW])
#define high_wmark_pages(z) (z->watermark[WMARK_HIGH])

#if MAX_NR_ZONES < 2
#define ZONES_SHIFT 0
#elif MAX_NR_ZONES <= 2
#define ZONES_SHIFT 1
#elif MAX_NR_ZONES <= 4
#define ZONES_SHIFT 2
#else
#define ZONES_SHIFT 9
#endif


#define MIGRATE_UNMOVABLE     0
#define MIGRATE_RECLAIMABLE   1
#define MIGRATE_MOVABLE       2
#define MIGRATE_PCPTYPES      3 /* The number of types on the pcp lists */
#define MIGRATE_RESERVE       3
#define MIGRATE_ISOLATE       4 /* can't allocate from here */
#define MIGRATE_TYPES         5

#define MAX_ZONELISTS         1
extern int page_group_by_mobility_disabled;
#define MAX_ZONELISTS 1

#define MAX_ORDER 11
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))
/* Maximum number of zones on a zonelist */

#define MAX_ZONES_PER_ZONELIST (MAX_NUMNODES * MAX_NR_ZONES)

#define LRU_BASE      0
#define LRU_ACTIVE    1
#define LRU_FILE      2

enum lru_list {
	LRU_INACTIVE_ANON = LRU_BASE,
	LRU_ACTIVE_ANON   = LRU_BASE + LRU_ACTIVE,
	LRU_INACTIVE_FILE = LRU_BASE + LRU_FILE,
	LRU_ACTIVE_FILE   = LRU_BASE + LRU_FILE + LRU_ACTIVE,
	LRU_UNEVICTABLE,
	NR_LRU_LISTS
};
#define for_each_lru(l) for(l = 0 ; l < NR_LRU_LISTS ; l++)

enum zone_stat_item {
	/* First 128 byte cacheline. */
	NR_FREE_PAGES,
	NR_LRU_BASE,
	NR_INACTIVE_ANON = NR_LRU_BASE, /* must match order of LRU_[IN]ACTIVE */
	NR_ACTIVE_ANON,
	NR_INACTIVE_FILE,
	NR_ACTIVE_FILE,
	NR_UNEVICTABLE,
	NR_MLOCK, /* mlock()ed pages found and moved off LRU */
	NR_ANON_PAGES, /* Mapped anonymous pages */
	NR_FILE_MAPPED, /* pagecache pages mapped into pagetables.
					 only modified from process context */
	NR_FILE_PAGES,
	NR_FILE_DIRTY,
	NR_WRITEBACK,
	NR_SLAB_RECLAIMABLE,
	NR_SLAB_UNRECLAIMABLE,
	NR_PAGETABLE, /* used for pagetables */
	NR_KERNEL_STACK,
	/* Second 128 byte cacheline */
	NR_UNSTABLE_NFS, /* NFS unstable pages */
	NR_BOUNCE,
	NR_VMSCAN_WRITE,
	NR_WRITEBACK_TEMP, /* Writeback using temporary buffers */
	NR_ISOLATED_ANON,  /* Temporary isolated pages from anon lru */
	NR_ISOLATED_FILE,  /* Temporary isolated pages from file lru */
	NR_SHMEM, /* shmem pages (included tmpfs / GEM pages)*/
	NR_DIRTIED, /* page dirtyings since bootup */
	NR_WRITTEN, /* page wirtings since bootup */
	NR_ANON_TRANSPARENT_HUGEPAGES,
	NR_VM_ZONE_STAT_ITEMS
};
/*
 * Free area
 */
struct free_area {
	struct list_head free_list[MIGRATE_TYPES];
	unsigned long nr_free;
};
struct zone_reclaim_stat {
	/*
	 * The pageout code in vmscan.c keeps track of how many of the
	 * mem/swap backed and file backed pages are referenced.
	 * The higher the rotated/scanned ratio,the more valuable 
	 * that cache is.
	 *
	 * The anon LRU stats live in [0],file LRU stats in [1]
	 */	
	unsigned long recent_rotated[2];
	unsigned long recent_scanned[2];
	/*
	 * accumulated for batching.
	 */
	unsigned long nr_saved_scan[NR_LRU_LISTS];
};
/*
 * We cache key information from each zonelist for smaller cacahe
 * footprint.
 */
struct zonelist_cache {
	unsigned short z_to_n[MAX_ZONES_PER_ZONELIST]; /* zone->nid */
	DECLARE_BITMAP(fullzones,MAX_ZONES_PER_ZONELIST); /* zone full? */
	unsigned long last_full_zap; /* when last zap'd(jiffies )*/
};
/*
 * This struct contains information about a zone in a zonelist.It is stored
 * here to avoid dereferences into large structures and lookups of tables.
 */
struct zoneref {
	struct zone *zone;
	int zone_idx;
};

struct zonelist {
	struct zonelist_cache *zlcache_ptr; /* NULL or &zlcache */
	struct zoneref _zonerefs[MAX_ZONES_PER_ZONELIST + 1];
};

/*
 * Per cpu struct. 
 */
struct per_cpu_pages {
	int count;    /* number of pages in the list */
	int high;     /* high watermark,emptying needed */
	int batch;    /* chunk size for buddy add/remove */
	/* List of pages,one per migrate type stored on the pcp-lists */
	struct list_head lists[MIGRATE_PCPTYPES];
};
struct per_cpu_pageset {
	struct per_cpu_pages pcp;
#define __per_cpu
};

struct zone {
	unsigned long watermark[NR_WMARK];
	struct zone_reclaim_stat reclaim_stat;
	unsigned long flags;
	struct pglist_data *zone_pgdat;
	const char *name;
	unsigned long zone_start_pfn;
	unsigned long spanned_pages;
	unsigned long present_pages;
	/*
	 * Fields commonly accessed by the page reclaim scanner.
	 */
	struct zone_lru {
		struct list_head list;
	} lru[NR_LRU_LISTS];
	/* 
	 * For Buddy System 
	 */
	struct free_area free_area[MAX_ORDER];
	/*
	 * Zone statistics.
	 */
	atomic_long_t vm_stat[NR_VM_ZONE_STAT_ITEMS];
	/*
	 * Flags for a pageblock_nr_pages block.See pageblock-flags.h
	 */
	unsigned long *pageblock_flags;

	struct per_cpu_pageset __percpu *pageset;

	unsigned long pages_scanned;   /* Since last reclaim */
	int all_unreclaimable;         /* All pages pinned */
	/* Fields commonly accessed by the page reclaim scanner */
	spinlock_t lru_lock;
	
	unsigned long lowmem_reserve[MAX_NR_ZONES];
	/*
	 * wait_table  -- the array holding the hash table
	 * wait_table_hash_nr_entries -- the size of the hash table array
	 * wait_table_bits -- wait_table_size == (1 << wait_table)
	 * 
	 * The purpose of all these is to keep track of the people
	 * waiting for a page to become available and make them
	 * runnable again when possible.The trouble is that this
	 * consumes a lot of space,epecially when so few things
	 * wait on pages at a given time.So instead of using
	 * per-page waitqueues,we use a waitqueue hash table.
	 *
	 * The bucket discipline is to sleep on the same queue when
	 * colliding and wake all in that wait queue when removing.
	 * When something wakes,it must check to be sure its page is
	 * truly available,a la thundering herd.The cost of a 
	 * collision is great,but given the expected load of the
	 * table,they should be so rare as to be outweighed by the 
	 * benefits from the saved space.
	 *
	 * __wait_on_page_locked() and unlock_page() in mm/filemap.c are the 
	 * primary users of these fields,and in mm/page_alloc.c
	 * free_area_init_core() performs the initialization of them.
	 */
	wait_queue_head_t *wait_table;
	unsigned long wait_table_hash_nr_entries;
	unsigned long wait_table_bits;
};

typedef struct pglist_data {
#ifndef CONFIG_NO_BOOTMEM
	struct bootmem_data *bdata;
#endif
    struct zone node_zones[MAX_NR_ZONES];
	struct zonelist node_zonelists[MAX_ZONELISTS];
    unsigned long node_id;
    unsigned long node_start_pfn;
    unsigned long node_spanned_pages;
    unsigned long node_present_pages;
    int nr_zones;
    int kswapd_max_order;
    struct page *node_mem_map;
#ifndef CONFIG_NO_CGROUP_MEM_RES_CTLR
	struct page_cgroup *node_page_cgroup;
#endif
	wait_queue_head_t kswapd_wait;

} pg_data_t;

extern struct pglist_data contig_pglist_data;

#define NODE_DATA(nid) (&(contig_pglist_data))
static inline int zone_movable_is_highmem(void)
{
#if defined(CONFIG_HIGHMEM) && defined(CONFIG_ARCH_POPULATES_NODE_MAP)
	return 0;
#else
	return 0;
#endif
}
/*
 * check zone is highmem.
 */
static inline int is_highmem_idx(enum zone_type idx)
{
#ifdef CONFIG_HIGHMEM
	return (idx == ZONE_HIGHMEM ||
			(idx == ZONE_MOVABLE && zone_movable_is_highmem()));
#else
	return 0;
#endif
}
/*
 * helper function to quickly check if a struct zone is a
 * highmem zone or not.This is an attempt to keep references
 * to ZONE_{DMA/NORMAL/HIGHMEM/etc} in general code to a minimum.
 */
static inline int is_highmem(struct zone *zone)
{
#ifndef CONFIG_HIGHMEM
	int zone_off = (char *)zone - (char *)zone->zone_pgdata->node_zones;

	return zone_off == ZONE_HIGHMEM * sizeof(*zone) ||
		   (zone_off == ZONE_MOVABLE * sizeof(*zone) &&
			zone_movable_is_highmem());
#else
	return 0;
#endif
}
/*
 * Check zone is normal zone.
 */
static inline int is_normal_idx(enum zone_type idx)
{
	return (idx == ZONE_NORMAL);
}
/*
 * zone_idx() return 0 for the ZONE_DMA zone,1 for the ZONE_NORMAL zone.etc
 */
#define zone_idx(zone) ((zone) - (zone)->zone_pgdat->node_zones)

#define for_each_zone_zonelist(zone,z,zlist,highidx) \
	for_each_zone_zonelist_nodemask(zone,z,zlist,highidx,NULL)

static inline int zonelist_zone_idx(struct zoneref *zoneref)
{
	return zoneref->zone_idx;
}

static inline struct zone *zonelist_zone(struct zoneref *zoneref)
{
	return zoneref->zone;
}
extern struct zoneref *next_zones_zonelist(struct zoneref *z,
				enum zone_type highest_zoneidx,
				nodemask_t *nodes,struct zone **zone);

#define high_wmark_pages(z)   (z->watermark[WMARK_HIGH])

/*
 * PAGE_ALLOC_COSTLY_ORDER is the order at which allocations are deemed
 * costly to service.That is between allocation orders which should
 * coelesce naturally under reasonable reclaim pressure and those which
 * will not
 */
#define PAGE_ALLOC_COSTLY_ORDER    3

#define MIGRATE_UNMOVABLE        0
#define MIGRATE_RECLAIMABLE      1
#define MIGRATE_MOVABLE          2
#define MIGRATE_PCPTYPES         3 /* The number of types on the pcp lists */
#define MIGRATE_RESERVE          3
#define MIGRATE_ISOLATE          4
#define MIGRATE_TYPES            5

#define for_each_migratetype_order(order,type)    \
	for(order = 0 ; order < MAX_ORDER ; order++)   \
		for(type = 0 ; type < MIGRATE_TYPES ; type++)

#define early_pfn_valid(pfn) (1)
#define early_pfn_in_nid(pfn,nid) (1)
unsigned long get_pageblock_flags_group(struct page *page,
		int start_bitidx,int end_bitidx);
static inline unsigned long get_pageblock_migratetype(struct page *page)
{
	return get_pageblock_flags_group(page,PB_migrate,PB_migrate_end);
}
/*
 * If it is possible to have holes within a MAX_ORDER_NR_PAGES,then
 * need to check pfn validility within that MAX_ORDER_NR_PAGES block.
 * pfn_valid_within() should be used in this case;we optimise this away
 * when we have no holes within a MAX_ORDER_NR_PAGES block.
 */
#define pfn_valid(x) (1)
#define pfn_valid_within(pfn) pfn_valid(pfn)

/*
 * Section
 */
#define SECTION_SIZE_BITS   28
#define PFN_SECTION_SHIFT   (SECTION_SIZE_BITS - PAGE_SHIFT)

#define pfn_to_section_nr(pfn) ((pfn) >> PFN_SECTION_SHIFT)
#define section_nr_to_pfn(sec) ((sec) << PFN_SECTION_SHIFT)
/*
 * Returns the first zone at or below highest_zoneidx within the allowed.
 *
 * This fucntion return the first zone at or below a given zone index that is 
 * within the allowed nodemask.The zoneref returned is a cursor that can be
 * used to iterate the zonelist with next_zones_zonelist by advancing it by
 * one before calling.
 */
static inline struct zoneref *first_zones_zonelist(struct zonelist *zonelist,
		enum zone_type highest_zoneidx,
		nodemask_t *nodes,
		struct zone **zone)
{
	return next_zones_zonelist(zonelist->_zonerefs,highest_zoneidx,nodes,
			zone);
}
extern struct zone *next_zone(struct zone *zone);
extern struct pglist_data *first_online_pgdat(void);
#define for_each_populated_zone(zone)                         \
	for(zone = (first_online_pgdat())->node_zones;            \
			zone;               \
			zone = next_zone(zone))        \
		if(!populated_zone(zone))         \
			; /* do nothing */         \
		else


#define sparse_init() do {} while(0)
#endif

