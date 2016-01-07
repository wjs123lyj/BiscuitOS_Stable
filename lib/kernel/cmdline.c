#include "../../include/linux/kernel.h"

/*
 * memparse - parse a string with mem suffixes into a number
 * 
 * Parses a string into a number.The number stored at @ptr is 
 * potentially suffixed with %K.
 */
unsigned long long memparse(const char *ptr,char **retptr)
{
	char *endptr;  /* local pointer to end of parsed string */

	unsigned long long ret = simple_strtoull(ptr,&endptr,0);

	switch(*endptr) {
		case 'G':
		case 'g':
			ret <<= 10;
		case 'M':
		case 'm':
			ret <<= 10;
		case 'K':
		case 'k':
			ret <<= 10;
			endptr++;
		default:
			break;
	}

	if(retptr)
		*retptr = endptr;

	return ret;
}
/*
 * get-option - Parse integer from an option string.
 * 
 * Read an int form an option string;if available accept subsequent
 * comma as well.
 * Return values:
 * 0 - no int in string.
 * 1 - int found,no subsequent comma
 * 2 - int found including a subsequent comma
 * 3 - hyphen found to denote a range
 */
int get_option(char **str,int *pint)
{
	char *cur = *str;

	if(!cur || !(*cur))
		return 0;
//	*pint = simple_strtol(cur,str,0);
	if(cur == *str)
		return 0;
	if(**str == ',') {
		(*str)++;
		return 2;
	}
	if(**str == '-')
		return 3;

	return 1;
}
