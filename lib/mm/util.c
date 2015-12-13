



/*
 * Allocate space for and copy an existing string.
 */
char *kstrdup(const char *s,gfp_t gfp)
{
	size_t len;
	char *buf;

	if(!s)
		return NULL;

	len = strlen(s) + 1;
	buf = kmalloc_track_caller(len,gfp);
	if(buf)
		memcpy(buf,s,len);
	return buf;
}
