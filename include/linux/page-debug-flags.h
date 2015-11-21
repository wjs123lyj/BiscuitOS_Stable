#ifndef _PAGE_DEBUG_FLAGS_H_
#define _PAGE_DEBUG_FLAGS_H_

/*
 * page->debug_flags bits:
 * PAGE_DEBUG_FLAG_POISON is set for poisoned page.This is used to 
 * implement generic debug pagealloc feature.The pages are filled with
 * poison patterns and set this flag after free_pages().The poisoned
 * pages are verified whether the patterns are not corrupted and clear
 * the flag before alloc_pages().
 */
enum page_debug_flags {
	PAGE_DEBUG_FLAG_POISON, /* Page is poisoned */
};

#endif
