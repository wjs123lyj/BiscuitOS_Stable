#include "linux/kernel.h"
#include "linux/slab.h"

/**
 * kstrdup - allocate space for and copy an existing string
 * @s: the string to duplicate
 * @gfp: the GFP mask used in the kmalloc() call when allocation memory
 */
char *kstrdup(const char *s,gfp_t gfp)
{
	size_t len;
	char *buf;

	if(!s)
		return NULL;

	len = strlen(s) + 1;
	buf = (char *)(unsigned long)kmalloc_track_caller(len,gfp);
	if(buf)
		memcpy(buf,s,len);
	return buf;
}
