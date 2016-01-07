#ifndef _SETUP_H_
#define _SETUP_H_


#define COMMAND_LINE_SIZE   1024
/*
 * Memory map description
 */
#ifndef CONFIG_BOTH_BANKS
#define NR_BANKS 4
#else
#define NR_BANKS 8
#endif

/*
 * The list ends with an ATAG_NONE node.
 */
#define ATAG_NONE  0x00000000

struct tag_header {
	__u32 size;
	__u32 tag;
};

/* The list must start with an ATAG_CORE node */
#define ATAG_CORE  0x54410001

struct tag_core {
	__u32 flags;    /* bit 0 = read-only */
	__u32 pagesize;
	__u32 rootdev;
};

/* it is allowed to have multiple ATAG_MEM nodes */
#define ATAG_MEM  0x54410002

struct tag_mem32 {
	__u32 size;
	__u32 start;  /* physical start address */
};

/* Footbridge memory clock */
#define ATAG_MEMCLK  0x41000402

struct tag_memclk {
	__u32 fmemclk;
};

/* VGA text type displays */
#define ATAG_VIDEOTEXT  0x54410003

struct tag_videotext {
	__u8 x;
	__u8 y;
	__u16 video_page;
	__u8  video_mode;
	__u8  video_cols;
	__u16 video_ega_bx;
	__u8  video_lines;
	__u8  video_isvga;
	__u16 video_points;
};

/* Describes how the ramdisk will be used in kernel */
#define ATAG_RAMDISK   0x54410004

struct tag_ramdisk {
	__u32 flags;    /* bit 0 = load,bit 1 = prompt */
	__u32 size;     /* decompressed ramdisk size in __kilo_bytes */
	__u32 start;    /* starting block of floppy-based RAM disk image */
};

/*
 * This one accidentally used virtual addresses - as such,
 * it's deprecated.
 */
#define ATAG_INITRD    0x54410005

/* Describes where the compressed ramdisk image lives (physical address) */
#define ATAG_INITRD2   0x54420005

struct tag_initrd {
	__u32 start;  /* physical start address */
	__u32 size;   /* size of compressed ramdisk image in bytes */
};

/* Board serial number."64 bits should be enough for every body "*/
#define ATAG_SERIAL   0x54410006

struct tag_serialnr {
	__u32 low;
	__u32 high;
};

/* Board revision */
#define ATAG_REVISION  0x5441007

struct tag_revision {
	__u32 rev;
};

/*
 * Initial values for vesafb-type framebuffers.
 */
struct tag_videolfb {
	__u16 lfb_width;
	__u16 lfb_height;
	__u16 lfb_depth;
	__u16 lfb_linelength;
	__u32 lfb_base;
	__u32 lfb_size;
	__u8  red_size;
	__u8  red_pos;
	__u8  green_size;
	__u8  green_pos;
	__u8  blue_size;
	__u8  blue_pos;
	__u8  rsvd_size;
	__u8  rsvd_pos;
};

/* Command line:\0 terminated string */
#define ATAG_CMDLINE  0x5441009

struct tag_cmdline {
	char cmdline[1];  /* this is the minimum size */
};

/* Acorn RiscPC specific information */
#define ATAG_ACORN    0x41000101

struct tag_acorn {
	__u32 memc_control_reg;
	__u32 vram_pages;
	__u8  sounddefault;
	__u8  adfsdrives;
};

struct tag {
	struct tag_header hdr;
	union {
		struct tag_core core;
		struct tag_mem32 mem;
		struct tag_videotext videotext;
		struct tag_ramdisk ramdisk;
		struct tag_initrd initrd;
		struct tag_serialnr serialnr;
		struct tag_revision revision;
		struct tag_videolfb videolfb;
		struct tag_cmdline cmdline;

		/*
		 * Acom specific
		 */
		struct tag_acorn acorn;

		/*
		 * DC2185 specific
		 */
		struct tag_memclk memclk;
	} u;

};

struct tagtable {
	__u32 tag;
	int (*parse)(const struct tag *);
};

#define tag_next(t)    ((struct tag *)((__u32 *)(t) + (t)->hdr.size))
#define tag_size(type) ((sizeof(struct tag_header) + sizeof(struct type)) >> 2)

#define for_each_tag(t,base)      \
	for(t = base ; t->hdr.size ; t = tag_next(t))

#define __tag __used __attribute__((__section__(".taglist.init")))
#define __tagtable(tag,fn)    \
	static struct tagtable __tagtable_##fn __tag = { tag , fn }

struct membank {
	unsigned int start;
	unsigned int size;
	unsigned int highmem;
};

struct meminfo {
	int nr_banks;
	struct membank bank[NR_BANKS];
};

extern struct meminfo meminfo;

#define for_each_bank(iter,mi) \
	for(iter = 0 ; iter < (mi)->nr_banks ; iter++)

#define bank_pfn_start(bank) phys_to_pfn((bank)->start)
#define bank_pfn_end(bank)   phys_to_pfn((bank)->start + (bank)->size)
#define bank_pfn_size(bank)  phys_to_pfn((bank)->size)
#define bank_phys_start(bank) (bank)->start
#define bank_phys_end(bank)   ((bank)->start + (bank)->size)
#define bank_phys_size(bank)  (bank)->size

#endif
