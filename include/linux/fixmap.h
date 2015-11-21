#ifndef _FIXMAP_H_
#define _FIXMAP_H_

/*
 * Nothing to fancy for now.
 *
 * On ARM we already have well known fixed virtual address imposed by
 * the architecture such as vector page which is located at 0xffff0000,
 * therefore a second level page table is already allocated covering
 * 0xffff0000 upwards.
 *
 * The cache flushing code in proc-xscale.S uses the virtual area between
 * 0xfffe000 and oxfffeffff.
 */

#define FIXADDR_START     0xfff00000UL
#define FIXADDR_TOP       0xfffe0000UL
#define FIXADDR_SIZE      (FIXADDR_TOP - FIXADDR_START)

#define FIX_KMAP_DEBIG    0
#define FIX_KMAP_END      (FIXADDR_SIZE >> PAGE_SHIFT)

#endif
