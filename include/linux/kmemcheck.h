#ifndef _KMEMCHECK_H_
#define _KMEMCHECK_H_

static inline bool kmemcheck_page_is_tracked(struct page *p)
{
	return false;
}

#endif
