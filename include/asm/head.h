#ifndef _HEAD_H_
#define _HEAD_H_
#include "../linux/page.h"

#define TEXT_OFFSET         0x00008000

#define KERNEL_RAM_VADDR	(PAGE_OFFSET + TEXT_OFFSET)
#define KERNEL_RAM_PADDR	(PHYS_OFFSET + TEXT_OFFSET)

#define swapper_pg_dir     (pgd_t *)(KERNEL_RAM_VADDR - 0x00004000)
#endif
