#ifndef _PGTABLE_HWDEF_H_
#define _PGTABLE_HWDEF_H_

/*
 * Hardware page table definitions.
 *
 * Level 1 descriptor (PMD)
 */
#define PMD_TYPE_MASK      (3 << 0)
#define PMD_TYPE_FAULT     (0 << 0)
#define PMD_TYPE_TABLE     (1 << 0)
#define PMD_TYPE_SECT      (2 << 0)
#define PMD_BIT4           (1 << 4)
#define PMD_DOMAIN(x)      ((x) << 5)
#define PMD_PROTECTION     (1 << 9)
/*
 * Section
 */
#define PMD_SECT_BUFFERABLE   (1 << 2)
#define PMD_SECT_CACHEABLE    (1 << 3)
#define PMD_SECT_XN           (1 << 4)
#define PMD_SECT_AP_WRITE     (1 << 10)
#define PMD_SECT_AP_READ      (1 << 11)
#define PMD_SECT_TEX(x)       ((x) << 12)
#define PMD_SECT_APX          (1 << 15)
#define PMD_SECT_S            (1 << 16)
#define PMD_SECT_nG           (1 << 17)
#define PMD_SECT_SUPER        (1 << 18)

#define PMD_SECT_UNCACHED     (0)
#define PMD_SECT_BUFFERED     (PMD_SECT_BUFFERABLE)
#define PMD_SECT_WT           (PMD_SECT_CACHEABLE)
#define PMD_SECT_WB           (PMD_SECT_CACHEABLE | PMD_SECT_BUFFERABLE)
#define PMD_SECT_MINICACHE    (PMD_SECT_TEX(1) | PMD_SECT_CACHEABLE)
#define PMD_SECT_WBWA         (PMD_SECT_TEX(1) | PMD_SECT_CACHEABLE |   \
							   PMD_SECT_BUFFERABLE)
#define PMD_SECT_NONSHARED_DEV (PMD_SECT_TEX(2))



#endif
