#ifndef _MM_H_
#define _MM_H_
#include "mmzone.h"
#include "mm_type.h"
#include "atomic.h"

#define NODES_SHIFT           0
#define SECTIONS_SHIFT        0

#define SECTIONS_WIDTH        SECTIONS_SHIFT
#define ZONES_WIDTH           ZONES_SHIFT
#define NODES_WIDTH           NODES_SHIFT
#define ZONEID_SHIFT          (NODES_SHIFT + ZONES_SHIFT)

#define ZONES_MASK            ((1UL << ZONES_WIDTH) - 1)
#define NODES_MASK            ((1UL << NODES_WIDTH) - 1)
#define ZONEID_MASK           ((1UL << ZONEID_SHIFT) - 1)


#define SECTIONS_PGOFF        ((sizeof(unsigned long) * 8) - SECTIONS_WIDTH)
#define NODES_PGOFF           (SECTIONS_PGOFF - NODES_WIDTH)
#define ZONES_PGOFF           (NODES_PGOFF - ZONES_WIDTH)

#define NODES_PGSHIFT         (NODES_PGOFF * (NODES_WIDTH != 0))
#define ZONES_PGSHIFT         (ZONES_PGOFF * (ZONES_WIDTH != 0))
/*
 * Set page link to zone.
 */
static inline void set_page_zone(struct page *page,enum zone_type zone)
{
	page->flags &= ~(ZONES_MASK << ZONES_PGSHIFT);
	page->flags |= (zone & ZONES_MASK) << ZONES_PGSHIFT;
}
/*
 * Set page link to node.
 */
static inline void set_page_node(struct page *page,unsigned long node)
{
	page->flags &= ~(NODES_MASK << NODES_PGSHIFT);
	page->flags |= (node & NODES_MASK) << NODES_PGSHIFT;
}
/*
 * Set page link into list.
 */
static inline void set_page_links(struct page *page,
		enum zone_type zone,unsigned long node,unsigned long pfn)
{
	set_page_zone(page,zone);
	set_page_node(page,node);
	/* Ignore section */
	//set_page_section(...);
}
/*
 * Get the nid via page.
 */
static inline int page_to_nid(struct page *page)
{
	return (page->flags >> NODES_PGSHIFT) & NODES_MASK;
}
/*
 * Get the zonenum via page.
 */
static inline enum zone_type page_zonenum(struct page *page)
{
	return (page->flags >> ZONES_PGSHIFT) & ZONES_MASK;
}
/*
 * Initialize the page_counter that count the number of user.
 */
static inline void init_page_count(struct page *page)
{
	atomic_set(&page->_count,1);
}
/*
 * Initialize the page_mapping that count the number of mapping.
 */
static inline void reset_page_mapcount(struct page *page)
{
	atomic_set(&(page)->_mapcount,-1);
}
#endif
