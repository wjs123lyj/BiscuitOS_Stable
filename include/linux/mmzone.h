#ifndef _MMZONE_H_
#define _MMZONE_H_
#include "config.h"
#include "bootmem.h"
#include "highmem.h"
#include "bitops.h"
#include "memblock.h"
#include "numa.h"

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


#if MAX_NR_ZONES < 2
#define ZONES_SHIFT 0
#elif MAX_NR_ZONES <= 2
#define ZONES_SHIFT 1
#elif MAX_NR_ZONES <= 4
#define ZONES_SHIFT 2
#else
#define ZONES_SHIFT 9
#endif

#define MIGRATE_TYPES   5
#define MAX_ZONELISTS   1
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
	NR_WRITEBASK,
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
} pg_data_t;

extern struct pglist_data contig_pglist_data;

#define NODE_DATA(nide) (&(contig_pglist_data))
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

#define high_wmark_pages(z)   (z->watermark[WMARK_HIGH])


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
#endif