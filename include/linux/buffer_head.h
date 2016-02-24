#ifndef _BUFFER_HEAD_H_
#define _BUFFER_HEAD_H_

#include "linux/mm_types.h"
#include "linux/list.h"
#include "linux/types.h"

/*
 * Historically,a buffer_head was used to map a single block
 * within a page,and of course as the unit of I/O through the
 * filesystem and block layers.Nowaydays the basic I/O unit
 * is the bio,and buffer_heads are used for extraction block
 * mappings(via a get_block_t call),for tracking state within
 * a page (via a page_mapping) and for wrapping bio submission
 * for backward compatibility reasons(e.g.submit_bh).
 */
struct buffer_head {
	unsigned long b_state; /* buffer state bitmap(see above)*/
	struct buffer_head *b_this_page; /* circulate list of page's buffers */
	struct page *b_page;   /* the page this bh is mapped to */

//	sector_t b_blocknr;    /* start block number */
	size_t b_size;    /* size of mapping */
	char *b_data;     /* pointer to data within the page */

	//struct block_device *b_dev;
	//bh_end_io_t *b_end_io;   /* I/O completion */
	void *b_private;   /* reserved for b_end_io */
	struct list_head b_assoc_buffers;    /* asssociated with another mapping */
	//struct address_space *b_assoc_map; /* mapping this buffer is */
										 /* associated with */
	atomic_t b_count;   /* users using this buffer_head */
};



#endif
