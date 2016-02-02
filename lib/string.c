#include "linux/kernel.h"
#include "linux/ctype.h"
#include "linux/debug.h"


/*
 * Works only for digits and letters,but small and fast.
 */
#define TOLOWER(x)  ((x) | 0x20)

static unsigned int simple_guess_base(const char *cp)
{
	if(cp[0] == '0') {
		if(TOLOWER(cp[1]) == 'X' && isxdigit(cp[2]))
			return 16;
		else
			return 8;
	} else {
		return 10;
	}
}
/*
 * Skip_space - Removes leading whitespace form @str
 */
char *skip_spaces(const char *str)
{
	while(isspace(*str))
	{
		++str;
	}
	return (char *)str;
}
/*
 * Simple_strtoull - convert a string to an unsigned long long.
 */
unsigned long long simple_strtoull(const char *cp,char **endp,
		unsigned int base)
{
	unsigned long long result = 0;

	if(!base)
		base = simple_guess_base(cp);

	if(base == 16 && cp[0] == '0' && TOLOWER(cp[1] == 'X'))
		cp += 2;

	while(isxdigit(*cp)) {
		unsigned int value;

		value = isdigit(*cp) ? *cp - '0' : TOLOWER(*cp) - 'a' + 10;
		if(value >= base)
			break;
		result = result * base + value;
		cp++;
	}
	if(endp)
		*endp = (char *)cp;

	return result;
}

/**
 * strnlen - FIND the length of a length-limited string
 * @s: The string to be sized
 * @count: The maximum number of bytes to search
 */
size_t strnlen(const char *s,size_t count)
{
	const char *sc;

	for(sc = s; count-- && *sc != '\0' ; ++sc)
		/* nothing */;
	return sc - s;
}
