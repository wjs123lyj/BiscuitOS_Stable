#ifndef _PAGEBLOCK_FLAGS_H_
#define _PAGEBLOCK_FLAGS_H_

#define pageblock_order   (MAX_ORDER - 1)
#define pageblock_nr_pages (1UL << pageblock_order)

/* Bit indices that affect a whole block of pages */
enum pageblock_bits {
	PB_migrate,
	PB_migrate_end = PB_migrate + 3 - 1,
		/* 3 bits required for migrate types */
	NR_PAGEBLOCK_BITS
};
#endif
