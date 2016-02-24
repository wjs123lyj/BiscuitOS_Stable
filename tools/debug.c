#include "linux/kernel.h"
#include "linux/mmzone.h"
#include "linux/bootmem.h"
#include "linux/setup.h"
#include "linux/memblock.h"
#include "linux/atomic.h"
#include "linux/list.h"
#include "linux/vmstat.h"

extern struct meminfo meminfo;
extern unsigned int memory_array0[CONFIG_BANK0_SIZE / BYTE_MODIFY];
extern unsigned int memory_array1[CONFIG_BANK1_SIZE / BYTE_MODIFY];

char *page_flags_names[] = {
	"PG_locked",
	"PG_error",
	"PG_referenced",
	"PG_uptodate",
	"PG_dirty",
	"PG_lru",
	"PG_active",
	"PG_slab",
	"PG_owner_priv_1",
	"PG_arch_1",
	"PG_reserved",
	"PG_private",
	"PG_writeback",
#ifdef CONFIG_PAGEFLAGS_EXTENDED
	"PG_head",
	"PG_tail",
#else
	"PG_compound",
#endif
	"PG_swapcache",
	"PG_mappedtodisk",
	"PG_reclaim",
	"PG_swapbacked",
	"PG_unevictable",
#ifdef CONFIG_MMU
	"PG_mlocked",
#endif
#ifdef CONFIG_ARCH_USES_PG_UNCACHED
	"PG_uncached",
#endif
#ifdef CONFIG_MEMORY_FAILURE
	"PG_hwpoison",
#endif
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	"PG_comound_lock",
#endif
};

/*
 * Get the name of page-flags
 */
void PageFlage(struct page *page,char *s)
{
	unsigned long flags = page->flags;
	unsigned long i = 0;

	mm_debug("[%s]Page->flags:",s);
	while(flags) {
		if(flags & 0x1)
			mm_debug(" %s",page_flags_names[i]);
		flags >>= 1;
		i++;
	}
	mm_debug("\n");
}

unsigned int high_to_low(unsigned int old)
{
	unsigned int hold = 0x00;
	int i;

	for(i = 0 ; i < 7 ; i++)
	{
		hold |= old & 0xF0000000;
		hold >>= 4;
		old <<= 4;
	}
	return hold | old;
}
/*
 * Memory debug tool
 */
/*
 * Show all region of Memory->type.
 */
void __R_show(struct memblock_type *type,char *type_name,char *s)
{
    int i;

    for(i = type->cnt - 1 ; i >= 0 ; i--)
    {
        mm_debug("%s.Region[%d][%p - %p]cnt[%p]max[%p][%s]\n",type_name,i,
                (void *)type->regions[i].base,
                (void *)(type->regions[i].base +
                    type->regions[i].size),
				(void *)type->cnt,(void *)type->max,s);
    }
}
void R_show(char *s)
{
	__R_show(&memblock.memory,"Memory",s);
	__R_show(&memblock.reserved,"Reserved",s);
	mm_debug("Memblock.current_limit %p\n",(void *)memblock.current_limit);
}
/*
 * Show some memory segment of Memory_array.
 */
void M_show(phys_addr_t start,phys_addr_t end)
{
    int i,j;
	unsigned int *memory_array;
	unsigned long phys_offset;
	unsigned int t1,t2;

#ifndef CONFIG_BOTH_BANKS
	if(end > MAX_BANK0_PHYS_ADDR || 
			start < CONFIG_BANK0_START)
	{
		mm_err("ERR:overflow!\n");
		return;
	}
	memory_array = memory_array0;
    phys_offset =  CONFIG_BANK0_START;
#else
	if((end > MAX_BANK1_PHYS_ADDR) || 
			(start < CONFIG_BANK0_START))
	{
		mm_err("ERR:overflow!\n");
		return;
	}
	if(start >= CONFIG_BANK1_START)
	{
		memory_array = memory_array1;
		phys_offset = CONFIG_BANK1_START;
	}
	else
	{
		memory_array = memory_array0;
		phys_offset = CONFIG_BANK0_START;
	}
#endif
	start -= phys_offset;
	end   -= phys_offset;

    printf("=============Memory=============\n");
    for(i = start / BYTE_MODIFY / 5; i <= end / BYTE_MODIFY / 5 ; i++)
    {
        printf("\n[%08x]",(unsigned int)(i * 5 * BYTE_MODIFY + phys_offset));
        for(j = 0 ; j < 5 ; j++)
            printf("\t%08x",high_to_low(memory_array[i * 5 + j]));
    }
    mm_debug("\n=====================================\n");
}
/*
 * Show all bitmap of all memory.
 */
extern void __init find_limits(unsigned int *min,unsigned int *max_low,
		unsigned int *max_high);

void __B_show(unsigned long *bitmap,char *s)
{
    phys_addr_t start_pfn,end_pfn,high_pfn;
    unsigned long i;
    unsigned long bytes;
    unsigned char *byte_char = (unsigned char *)bitmap;
    
	/*
	 * Get the limit of normal memory and high memory.
	 */
	find_limits(&start_pfn,&end_pfn,&high_pfn);
    bytes = (end_pfn - start_pfn + 7) / 8;

    mm_debug("=============== BitMap[%s] =============\n",s);
    for(i = 0 ; i < 40 ; i++)
    {
        unsigned char byte = byte_char[i];
        int j;

        mm_debug("\nByte[%ld] ",i);
        for(j = 0 ; j < 8 ; j++)
        {
            unsigned char bit = byte & 0x01;

            mm_debug("\t%ld ",(unsigned long)bit);
            byte >>= 1;
        }

    }
    mm_debug("\n||");
    for(i = bytes - 20 ; i < bytes ; i++)
    {
        unsigned char byte = byte_char[i];
        int j;

        mm_debug("\nByte[%ld] ",i);
        for(j = 0 ; j < 8 ; j++)
        {
            unsigned char bit = byte & 0x01;

            mm_debug("\t%ld ",(unsigned long)bit);
            byte >>= 1;
        }
    }
    mm_debug("\n======================================\n");
}
void B_show(char *s)
{
	struct pglist_data *pgdat;

	pgdat = &contig_pglist_data;
	__B_show(pgdat->bdata->node_bootmem_map,s);
}
/*
 * Show membank
 */
void BK_show(char *s)
{
	int i;

	mm_debug("=================MemoryBank======================\n");
	for(i = 0 ; i < meminfo.nr_banks ; i++)
	{
		mm_debug("BANK[%d]\tregion[%08x - %08x]\tsize[%08x]highmem[%u][%s]\n",
				i,meminfo.bank[i].start,
				meminfo.bank[i].start + meminfo.bank[i].size,
				meminfo.bank[i].size,
				meminfo.bank[i].highmem,s);
	}
	mm_debug("=================================================\n");
}
/*
 * This function is used to show bitmap.
 */
void BITMAP_show(unsigned int *bitmap,unsigned long bits)
{
	unsigned long bytes = BITS_TO_LONGS(bits);
	int i;

	if(bitmap == NULL)
		return;

	mm_debug("========Bitmap===========\n");
	for(i = 0 ; i < bytes ; i++)
	{
		int j;
		int mask;

		mask = bitmap[i];
		mm_debug("[%d]",(i * 8));
		for(j = 0 ; j < 8 ; j++)
		{
			mm_debug(" %d",(mask & 0x1));
			mask >>= 1;
		}
		mm_debug("\n");
	}
}
/*
 * Those functions are used to show themself information quickly.
 * Use ST_xx(struct *xx);
 */
/*
 * Struct bootmem_data.
 */
void ST_bootmem_data(struct bootmem_data *bdata)
{
	if(bdata == NULL)
		return;
	mm_debug("Bootmem_data.node_min_pfn %p\n",(void *)bdata->node_min_pfn);
	mm_debug("Bootmem_data.node_low_pfn %p\n",(void *)bdata->node_low_pfn);
}
/*
 * Struct zone_reclaim_stat
 */
void ST_zone_reclaim_stat(struct zone_reclaim_stat *zr)
{
	if(zr == NULL)
		return;
	mm_debug("zr.recent_rotated[0] %p\n",(void *)zr->recent_rotated[0]);
	mm_debug("zr.recent_rotated[1] %p\n",(void *)zr->recent_rotated[1]);
	mm_debug("zr.recent_scanned[0] %p\n",(void *)zr->recent_scanned[0]);
	mm_debug("zr.recent_scanned[1] %p\n",(void *)zr->recent_scanned[1]);
}
/*
 * Struct zone node_zones
 */
void ST_node_zones(struct zone *zone,int nr)
{
	int i;

	if(zone == NULL)
		return;
	for(i = 0 ; i < nr ; i++)
	{
		struct zone *z;

		z = zone + i;
		mm_debug("zone.watermark[WMARK_MIN] %p\n",
				(void *)z->watermark[WMARK_MIN]);
		mm_debug("zone.watermark[WMARK_LOW] %p\n",
				(void *)z->watermark[WMARK_LOW]);
		mm_debug("zone.watermark[WMARK_HIGH] %p\n",
				(void *)z->watermark[WMARK_HIGH]);
		ST_zone_reclaim_stat(&z->reclaim_stat);
		mm_debug("zone.flags %p\n",(void *)z->flags);
		mm_debug("zone.name %s\n",z->name);
		mm_debug("zone.zone_start_pfn %p\n",(void *)z->zone_start_pfn);
		mm_debug("zone.present_pages %p\n",(void *)z->present_pages);
	}
}
/*
 * Struct zone
 */
void ST_zone(struct zone *zone)
{
	if(zone == NULL)
		return;
	ST_node_zones(zone,1);
}
/*
 * Struct zonelist_cache
 */
void ST_zonelist_cache(struct zonelist_cache *zc)
{
	int i;

	if(zc == NULL)
		return;

	for(i = 0 ; i < MAX_ZONES_PER_ZONELIST ; i++)
	{
		mm_debug("zonelist_cache.z_to_n[%d] %u\n",i,
				(zc->z_to_n[i]));
	}
	BITMAP_show(zc->fullzones,MAX_ZONES_PER_ZONELIST);
	mm_debug("zonelist_cache.last_full_zap[%p]\n",
			(void *)zc->last_full_zap);
}
/*
 * Struct zonelist
 */
void ST_zonelist(struct zonelist *zl)
{
	if(zl == NULL)
		return;
	ST_zonelist_cache(zl->zlcache_ptr);
}
/*
 * Struct page
 */
void ST_page(struct page *page)
{
	mm_debug("page->flags %ld\n",page->flags);
	mm_debug("page->_count %d\n",atomic_read(&page->_count));
}
/*
 * Struct pglist_data
 */
void ST_pglist_data(void)
{
	struct pglist_data *pgdat = NODE_DATA(0);

	ST_bootmem_data(pgdat->bdata);
	ST_node_zones(pgdat->node_zones,MAX_NR_ZONES);
	ST_zonelist(pgdat->node_zonelists);
	mm_debug("pglist.node_id %ld\n",pgdat->node_id);
	mm_debug("pglist.node_start_pfn %p\n",(void *)pgdat->node_start_pfn);
	mm_debug("pglist.node_spanned_pages %p\n",
			(void *)pgdat->node_spanned_pages);
	mm_debug("pglist.node_present_pages %p\n",(void *)pgdat->node_present_pages);
	mm_debug("pglist.nr_zones %d\n",pgdat->nr_zones);
	ST_page(pgdat->node_mem_map);
}
/*
 * Virtual area show.
 */
void V_show(void *addr,unsigned long size)
{
	unsigned int *start = (unsigned int *)addr;
	unsigned long data  = (unsigned long)(unsigned int *)addr;
	unsigned long bytes = (size + 7) / 8;
	unsigned int i,j;

	for(i = 0 ; i <= bytes / 6 ; i++)
	{
		mm_debug("[%p] ", (void *)(unsigned long)(i * 6 + data));
		for(j = 0 ; j < 6 ; j++)
		{
			mm_debug(" %p ",(void *)(unsigned long)(start[i * 6 + j]));
		}
		mm_debug("\n");
	}
}

/*
 * Get the binary
 */
void binary(unsigned int data)
{
	char value[33];
	int i;
	for(i = 0 ; i < 32 ; i++) {
		if(data & 0x1)
			value[31 - i] = '1';
		else
			value[31 - i] = '0';
		data >>= 1;
	}
	value[32] = '\0';
	mm_debug("binary %s\n",value);
}
void buddy_free(char *name)
{
	int i,j;
	struct zone *zone = &NODE_DATA(0)->node_zones[0];

	for(i = 0 ; i < MAX_ORDER ; i++) {
		struct free_area *free = &zone->free_area[i];
		struct list_head *list;

		mm_debug("[%s]Order %d nr_free %p\n",name,i,(void *)free->nr_free);
		for(j = 0 ; j < MIGRATE_TYPES ; j++) {
			int total = 0;
			if(list_empty(&free->free_list[j]))
				total = 0;
			else 
				list_for_each(list,&free->free_list[j])
					total += 1;
			mm_debug("|-->MIGRATETYPE %d total %p\n",
					j,(void *)(unsigned long)total);
		}
	}
}
void stop(void)
{
	while(1);
}

char *zone_stat_item_name[] = {
	"NR_FREE_PAGES",
	"NR_LRU_BASE",
	"NR_INACTIVE_ANON",
	"NR_ACTIVE_ANON",
	"NR_INACTIVE_FILE",
	"NR_ACTIVE_FILE",
	"NR_UNEVICTABLE",
	"NR_MLOCK",
	"NR_ANON_PAGES",
	"NR_FILE_MAPPED",
	"NR_FILE_PAGES",
	"NR_FILE_DIRTY",
	"NR_WRITEBACK",
	"NR_SLAB_RECLAIMABLE",
	"NR_SLAB_UNRECLAIMABLE",
	"NR_PAGETABLE",
	"NR_KERNEL_STACK",
	"NR_UNSTABLE_NFS",
	"NR_BOUNCE",
	"NR_VMSCAN_WRITE",
	"NR_WRITEBACK_TEMP",
	"NR_ISOLATED_ANON",
	"NR_ISOLATED_FILE",
	"NR_SHMEM",
	"NR_DIRTIED",
	"NR_WRITTEN",
#ifndef CONFIG_NUMA
	"NUMA_HIT",
	"NUMA_MISS",
	"NUMA_FOREIGN",
	"NUMA_INTERLEAVE_HIT",
	"NUMA_LOCAL",
	"NUMA_OTHER",
#endif
	"NR_ANON_TRANSPARENT_HUGEPAGES"
};

/*
 * Get all information of memory on some zone.
 */
void zone_information(char *name)
{
	int i;
	struct zone *zone = NODE_DATA(0)->node_zones;

	mm_debug("===========%s============\n",name);
	mm_debug("zone %s\n",zone->name);
	for(i = 0 ; i < NR_VM_ZONE_STAT_ITEMS ; i++) 
		mm_debug("%s:\t%p\n",zone_stat_item_name[i],
				(void *)zone_page_state(zone,i));
	mm_debug("============END=============\n");
	
}



