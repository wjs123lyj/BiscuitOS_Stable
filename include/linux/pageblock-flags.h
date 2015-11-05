#ifndef _PAGEBLOCK_FLAGS_H_
#define _PAGEBLOCK_FLAGS_H_
#include "mmzone.h"

#define pageblock_order   (MAX_ORDER - 1)
#define pageblock_nr_pages (1UL << pageblock_order)
#endif
