#include "linux/kernel.h"
#include "linux/setup.h"
#include "linux/page.h"
#include "linux/debug.h"

struct param_struct {
	union {
		struct {
			unsigned long page_size;  
			unsigned long nr_pages;
			unsigned long ramdisk_size;
			unsigned long flags;
#define FLAG_READONLY  1
#define FLAG_RDLOAD    4
#define FLAG_RDPROMPT  8
			unsigned long rootdev;
			unsigned long video_num_cols;
			unsigned long video_num_rows;
			unsigned long video_x;
			unsigned long video_y;
			unsigned long memc_control_reg;
			unsigned long sounddefault;
			unsigned long adfsdrives;
			unsigned long bytes_per_char_h;
			unsigned long bytes_per_char_v;
			unsigned long pages_in_bank[4];
			unsigned long pages_in_vram;
			unsigned long initrd_start;
			unsigned long initrd_size;
			unsigned long rd_start;
			unsigned long system_rev;
			unsigned long system_serial_low;
			unsigned long system_serial_high;
			unsigned long mem_fclk_21285;
		} s;
		char unused[256];
	} u1;
	union {
		char paths[8][128];
		struct {
			unsigned long magic;
			char n[1024 - sizeof(unsigned long)];
		} s;
	} u2;
	char commandline[COMMAND_LINE_SIZE];
};

static void __init build_tag_list(struct param_struct *params,void *taglist)
{
	struct tag *tag = taglist;

	if(params->u1.s.page_size != PAGE_SIZE) {
		mm_warn("Warning:bad configuration page, "
				"trying to continue\n");
		return;
	}

	mm_debug("Convertion old-style param struct to taglist\n");

	tag->hdr.tag = ATAG_CORE;
	tag->hdr.size = tag_size(tag_core);
	tag->u.core.flags = params->u1.s.flags & FLAG_READONLY;
	tag->u.core.pagesize = params->u1.s.page_size;
	tag->u.core.rootdev = params->u1.s.rootdev;

	tag = tag_next(tag);
	tag->hdr.tag = ATAG_RAMDISK;
	tag->hdr.size = tag_size(tag_ramdisk);
	tag->u.ramdisk.flags = (params->u1.s.flags & FLAG_RDLOAD ? 1 : 0) |
		(params->u1.s.flags & FLAG_RDPROMPT ? 2 : 0);
	tag->u.ramdisk.size = params->u1.s.ramdisk_size;
	tag->u.ramdisk.start = params->u1.s.rd_start;

	tag = tag_next(tag);
	tag->hdr.tag = ATAG_INITRD;
	tag->hdr.size = tag_size(tag_initrd);
	tag->u.initrd.start = params->u1.s.initrd_start;
	tag->u.initrd.size  = params->u1.s.initrd_size;

	tag = tag_next(tag);
	tag->hdr.tag = ATAG_SERIAL;
	tag->hdr.size = tag_size(tag_serialnr);
	tag->u.serialnr.low = params->u1.s.system_serial_low;
	tag->u.serialnr.high = params->u1.s.system_serial_high;

	tag = tag_next(tag);
	tag->hdr.tag = ATAG_REVISION;
	tag->hdr.size = tag_size(tag_revision);
	tag->u.revision.rev = params->u1.s.system_rev;

	tag = tag_next(tag);
	tag->hdr.tag = ATAG_CMDLINE;
	tag->hdr.size = (strlen(params->commandline) + 3 + 
			sizeof(struct tag_header)) >> 2;
	strcpy(tag->u.cmdline.cmdline,params->commandline);

	tag = tag_next(tag);
	tag->hdr.tag = ATAG_NONE;
	tag->hdr.size = 0;

	memmove(params,taglist,((unsigned long)tag) - ((unsigned long)taglist) +
			sizeof(struct tag_header));
}

void __init convert_to_tag_list(struct tag *tags)
{
	struct param_struct *params = (struct param_struct *)tags;
	build_tag_list(params,&params->u2);
}
