#define _UNUSE_STD_
#include "linux/kernel.h"
#include "linux/types.h"
#include "linux/page.h"
#include "asm/sections.h"
#include "linux/acenv.h"
#include "linux/ioport.h"
#include "linux/debug.h"
#include <string.h>

/* Works only for digits and letters,but small and fast */
#define TOLOWER(x) ((x) | 0x20)

#define ZEROPAD     1   /* pad with zero */  
#define SIGN        2   /* unsigned/signed long */
#define PLUS        4   /* show plus */
#define SPACE       8   /* space if plus */
#define LEFT        16  /* left justified */
#define SMALL       32  /* use lowercase in hex */
#define SPECIAL     64  /* prefix hex with '0x',octal with '0' */

enum format_type {
	FORMAT_TYPE_NONE,  /* just a string part */
	FORMAT_TYPE_WIDTH,
	FORMAT_TYPE_PRECISION,
	FORMAT_TYPE_CHAR,
	FORMAT_TYPE_STR,
	FORMAT_TYPE_PTR,
	FORMAT_TYPE_PERCENT_CHAR,
	FORMAT_TYPE_INVALID,
	FORMAT_TYPE_LONG_LONG,
	FORMAT_TYPE_ULONG,
	FORMAT_TYPE_LONG,
	FORMAT_TYPE_UBYTE,
	FORMAT_TYPE_BYTE,
	FORMAT_TYPE_USHORT,
	FORMAT_TYPE_SHORT,
	FORMAT_TYPE_UINT,
	FORMAT_TYPE_INT,
	FORMAT_TYPE_NRCHARS,
	FORMAT_TYPE_SIZE_T,
	FORMAT_TYPE_PTRDIFF
};

struct printf_spec {
	u8 type;          /*format_type enum */
	u8 flags;         /* flags to number() */
	u8 base;          /* number base,8,10 or 16 only */
	u8 qualifier;     /* number qualifier,one of 'hHILtzZ' */
	s16 field_width;  /* width of output field */
	s16 precision;    /* # of digits/chars */
};

#define do_div(a,b) ((a) / (b))
static unsigned int simple_guess_base(const char *cp)
{
	if(cp[0] == '0') {
		if(TOLOWER(cp[1] == 'x' && isxdigit(cp[2])))
			return 16;
		else
			return 8;
	} else
		return 10;
}

static int skip_atoi(const char **s)
{
	int i = 0;

	while(isdigit(**s))
		i = i * 10 + *((*s)++) - '0';

	return i;
}
static char *string(char *buf,char *end,const char *s,
		        struct printf_spec spec);


static char *put_dec_full(char *buf,unsigned q)
{
	/*
	 * BTW,if q is in [0,9999],8-bit ints will be enough,
	 * But anyway,gcc produces better code with full-sized ints.
	 */
	unsigned d3,d2,d1,d0;
	d1 = (q >> 4) & 0xf;
	d2 = (q >> 8) & 0xf;
	d3 = (q >> 12);

	/*
	 * possible ways to approx,divide by 10
	 */
	d0 = 6 * (d3 + d2 + d1) + (q & 0xf);
	q = (d0 * 0xcd) >> 11;
	d0 = d0 - 10 * q;
	*buf++ = d0 + '0';
	d1 = q + 9 * d3 + 5 * d2 + d1;
	q = (d1 * 0xcd) >> 11;
	d1 = d1 - 10 * q;
	*buf++ = d1 + '0';

	d2 = q + 2 *d2;
	q = (d2 * 0xd) >> 7;
	d2 = d2 - 10 * q;
	*buf++ = d2 = '0';

	d3 = q + 4 * d3;
	q = (d3 * 0xcd) >> 11;
	d3 = d3 - 10 * q;
	*buf++ = d3 + '0';
	*buf++ = q + '0';

	return buf;
}

/*
 * Decimal conversion is by far the most typical,and is used
 * for /proc and /sys data.This directly impacts e.g. top performance
 * with many processes running.We optimize it for speed 
 * using ....
 */
static char *put_dec_trunc(char *buf,unsigned q)
{
	unsigned d3,d2,d1,d0;
	d1 = (q >> 4) & 0xf;
	d2 = (q >> 8) & 0xf;
	d3 = (q >> 12);

	d0 = 6 * (d3 + d2 + d1) + (q & 0xf);
	q = (d0 * 0xcd) >> 11;
	d0 = d0 - 10 * q;
	*buf++ = d0 + '0'; /* least significant digit */
	d1 = q + 9 * d3 + 5 * d2 + d1;
	if(d1 != 0) {
		q = (d1 * 0xcd) >> 11;
		d1 = d1 - 10 * q;
		*buf++ = d1 + '0'; /* next digit */

		d2 = q + 2 * d2;
		if((d2 != 0) || (d3 != 0)) {
			q = (d2 * 0xd) >> 7;
			d2 = d2 - 10 * q;
			*buf++ = d2 + '0'; /* next digit */

			d3 = q + 4 * d3;
			if(d3 != 0) {
				q = (d3 * 0xcd) >> 11;
				d3 = d3 - 10 * q;
				*buf++ = d3 + '0'; /* next digit */
				if(q != 0)
					*buf++ = q + '0'; /* most sign.digit */
			}
		}
	}

	return buf;
}

static char *put_dec(char *buf,unsigned long long num)
{
	while(1) {
		unsigned rem;
		if(num < 100000)
			return put_dec_trunc(buf,num);
		rem = do_div(num,100000);
		buf = put_dec_full(buf,rem);
	}
}

static char *number(char *buf,char *end,unsigned long long num,
		struct printf_spec spec)
{
	/* we are called with base 8,10 or 16,only,thus don't need */
	static const char digits[16] = "0123456789ABCDEF";

	char tmp[66];
	char sign;
	char locase;
	int need_pfx = ((spec.flags & SPECIAL) && spec.base != 10);
	int i;

	/*
	 * Locase = 0 or 0x20.ORing digits or letters with 'locase'
	 * produces same digits or letters.
	 */
	locase = (spec.flags & SMALL);
	if(spec.flags & LEFT)
		spec.flags & ~ZEROPAD;
	sign = 0;
	if(spec.flags & SIGN) {
		if((signed long long)num < 0) {
			sign = '-';
			num = -(signed long long)num;
			spec.field_width--;
		} else if(spec.flags & PLUS) {
			sign = ' ';
			spec.field_width--;
		}
	}
	if(need_pfx) {
		spec.field_width--;
		if(spec.base == 16)
			spec.field_width--;
	}

	/* generate full string in tmp[],in reverse order */
	i = 0;
	if(num == 0)
		tmp[i++] = '0';
	else if(spec.base != 10) {
		int mask = spec.base - 1;
		int shift = 3;

		if(spec.base == 16)
			shift = 4;
		do {
			tmp[i++] = (digits[((unsigned char)num) & mask] | locase);
			num >>= shift;
		} while(num);
	} else {
		i = put_dec(tmp,num) - tmp;
	}

	/* printing 100 using %2d gives "100",not "00" */
	if(i > spec.precision)
		spec.precision = i;
	/* leading space padding */
	spec.field_width -= spec.precision;
	if(!(spec.flags & (ZEROPAD + LEFT))) {
		while(--spec.field_width >= 0) {
			if(buf < end)
				*buf = ' ';
			++buf;
		}
	}
	/* sign */
	if(sign) {
		if(buf < end)
			*buf = sign;
		++buf;
	}
	/* "0x" / "0" prefix */
	if(need_pfx) {
		if(buf < end)
			*buf = '0';
		++buf;
		if(spec.base == 16) {
			if(buf < end)
				*buf = ('X' | locase);
			++buf;
		}
	}
	/* zero or space padding */
	if(!(spec.flags & LEFT)) {
		char c = (spec.flags & ZEROPAD) ? '0' : ' ';
		while(--spec.field_width >= 0) {
			if(buf < end)
				*buf = c;
			++buf;
		} 
	}
	/* hmm even more zero padding */
	while(i <= --spec.precision) {
		if(buf < end)
			*buf = '0';
		++buf;
	} 
	/* actual digits of result */
	while(--i >= 0) {
		if(buf < end) 
			*buf = tmp[i];
		++buf;
	}
	/* trailing space padding */
	while(--spec.field_width >= 0) {
		if(buf < end)
			*buf = ' ';
		++buf;
	}

	return buf;
}

static char *symbol_string(char *buf,char *end,void *ptr,
		struct printf_spec spec,char ext)
{
	unsigned long value = (unsigned long)ptr;

	spec.field_width = 2 * sizeof(void *);
	spec.flags |= SPECIAL | SMALL | ZEROPAD;
	spec.base = 16;

	return number(buf,end,value,spec);
}

static char *resource_string(char *buf,char *end,struct resource *res,
		struct printf_spec spec,const char *fmt)
{
#ifndef IO_RSRC_PRINTK_SIZE
#define IO_RSRC_PRINTK_SIZE 6
#endif

#ifndef MEM_RSRC_PRINTK_SIZE
#define MEM_RSRC_PRINTK_SIZE 10
#endif
	static const struct printf_spec io_spec = {
		.base = 16,
		.field_width = IO_RSRC_PRINTK_SIZE,
		.precision = -1,
		.flags = SPECIAL | SMALL | ZEROPAD,
	};
	static const struct printf_spec mem_spec = {
		.base = 16,
		.field_width = MEM_RSRC_PRINTK_SIZE,
		.precision = -1,
		.flags = SPECIAL | SMALL | ZEROPAD,
	};
	static const struct printf_spec bus_spec = {
		.base = 16,
		.field_width = 2,
		.precision = -1,
		.flags = SMALL | ZEROPAD,
	};
	static const struct printf_spec dec_spec = {
		.base = 10,
		.precision = -1,
		.flags = 0,
	};
	static const struct printf_spec str_spec = {
		.field_width = -1,
		.precision = 10,
		.flags = LEFT,
	};
	static const struct printf_spec flag_spec = {
		.base = 16,
		.precision = -1,
		.flags = SPECIAL | SMALL,
	};

#define RSRC_BUF_SIZE ((2 * sizeof(resource_size_t)) + 4)
#define FLAG_BUF_SIZE (2 * sizeof(res->flags))
#define DECODED_BUF_SIZE sizeof("[mem - 64bit pref window disabled]")
#define RAW_BUF_SIZE     sizeof("[mem -flags 0x]")
	char sym[max(2 * RSRC_BUF_SIZE + DECODED_BUF_SIZE,
			2 * RSRC_BUF_SIZE + FLAG_BUF_SIZE + RAW_BUF_SIZE)];
	char *p = sym,*pend = sym + sizeof(sym);
	int decode = (fmt[0] == 'R') ? 1 : 0;
	const struct printf_spec *specp;

	*p++ = '[';
	if(res->flags & IORESOURCE_IO) {
		p = string(p,pend,"io  ",str_spec);
		specp = &io_spec;
	} else if(res->flags & IORESOURCE_MEM) {
		p = string(p,pend,"mem ",str_spec);
		specp = &mem_spec;
	} else if(res->flags & IORESOURCE_IRQ) {
		p = string(p,pend,"irq ",str_spec);
		specp = &dec_spec;
	} else if(res->flags & IORESOURCE_DMA) {
		p = string(p,pend,"dma ",str_spec);
		specp = &dec_spec;
	} else if(res->flags & IORESOURCE_BUS) {
		p = string(p,pend,"bus ",str_spec);
		specp = &bus_spec;
	} else {
		p = string(p,pend,"???",str_spec);
		specp = &mem_spec;
		decode = 0;
	}
	p = number(p,pend,res->start,*specp);
	if(res->start != res->end) {
		*p++ = '-';
		p = number(p,pend,res->end,*specp);
	}
	if(decode) {
		if(res->flags & IORESOURCE_MEM_64)
			p = string(p,pend," 64bit",str_spec);
		if(res->flags & IORESOURCE_PREFETCH)
			p = string(p,pend," pref",str_spec);
		if(res->flags & IORESOURCE_WINDOW)
			p = string(p,pend,"window",str_spec);
		if(res->flags & IORESOURCE_DISABLED)
			p = string(p,pend,"disabled",str_spec);
	} else {
		p = string(p,pend," flags",str_spec);
		p = number(p,pend,res->flags,flag_spec);
	}
	*p++ = ']';
	*p = '\0';

	return string(buf,end,sym,spec);

}


static char *mac_address_string(char *buf,char *end,u8 *addr,
		struct printf_spec spec,const char *fmt)
{
	char mac_addr[sizeof("xx:xx:xx:xx:xx:xx:")];
	char *p = mac_addr;
	int i;
	char separator;

	if(fmt[1] == 'F') { /* FDDI canonical format*/
		separator = '-';
	} else {
		separator = ':';
	}

	for(i = 0 ; i < 6 ; i++) {
		p = pack_hex_byte(p,addr[i]);
		if(fmt[0] == 'M' && i != 5)
			*p++ = separator;
	}
	*p = '\0';

	return string(buf,end,mac_addr,spec);
}

/*
 * Show a %p thing.A kernel extension is that the %p is followed
 * by an extra set of alphanumeric characters that are extended format
 * specifiers.
 */
static char *pointer(const char *fmt,char *buf,char *end,
		void *ptr,struct printf_spec spec)
{
	if(!ptr) {
		/*
		 * print(null) with same width as a pointer so it makes
		 * tabular output look nice.
		 */
		if(spec.field_width == -1)
			spec.field_width = 2 * sizeof(void *);
		return string(buf,end,"(null)",spec);
	}

	switch(*fmt) {
		case 'F':
		case 'f':
			ptr = dereference_function_descriptor(ptr);
			/* Fallthrough */
		case 'S':
		case 's':
			return symbol_string(buf,end,ptr,spec,*fmt);
		case 'R':
		case 'r':
			return resource_string(buf,end,ptr,spec,fmt);
		case 'M':    /* Color separated */
		case 'm':    
			return mac_address_string(buf,end,ptr,spec,fmt);
#if 0
		case 'I':
		case 'i':

			switch(fmt[1]) {
				case '6':
					return NULL;//ip6_addr_string(buf,end,ptr,spec,fmt);
				case '4':
					return NULL;//ip4_addr_string(buf,end,ptr,spec,fmt);
			}
			break;
		case 'U':
			return NULL; //uuid_string(buf,end,ptr,spec,fmt);
		case 'V':
			return buf + vsnprintf(buf,end - buf,
					((struct va_format *)ptr)->fmt,
					*(((struct va_format *)ptr)->val));
		case 'K':
			/*
			 * %pK cannot be used in IRQ context becase its test
			 * for CAP_SYSLOG would be meaningless.
			 */
			if(in_irq() || in_serving_softing() || in_nmi()) {
				if(spec.field_width = -1)
					spec.field_width = 2 * sizeof(void *);
				return string(buf,end,"pK-error",spec);
			} else if((kptr_restrict == 0) ||
					(kptr_restrict == 1 && 
					 has_capability_noaudit(current,CAP_SYSLOG)))
				break;

			if(spec.field_width == -1) {
				spec.field_width = 2 * sizeof(void *);
				spec.flags |= ZEROPAD;
			}
			return number(buf,end,0,spec);
#endif
	}
	spec.flags |= SMALL;
	if(spec.field_width == -1) {
		spec.field_width = 2 * sizeof(void *);
		spec.flags |= ZEROPAD;
	}
	spec.base = 16;

	return number(buf,end,(unsigned long)ptr,spec);
}
static char *string(char *buf,char *end,const char *s,
		struct printf_spec spec)
{
	int len,i;

	if((unsigned long)s < PAGE_SIZE)
		s = "(null)";

	len = strnlen(s,spec.precision);

	if(!(spec.flags & LEFT)) {
		while(len < spec.field_width--) {
			if(buf < end)
				*buf = ' ';
			++buf;
		}
	}
	for(i = 0 ; i < len ; i++) {
		if(buf < end)
			*buf = *s;
		++buf;
		++s;
	}
	while(len < spec.field_width--) {
		if(buf < end)
			*buf = ' ';
		++buf;
	}

	return buf;
}
/*
 * Helper function to decode printf style format.
 */
static int format_decode(const char *fmt,struct printf_spec *spec)
{
	const char *start = fmt;

	/* We finished early by reading the field width */
	if(spec->type == FORMAT_TYPE_WIDTH) {
		if(spec->field_width < 0) {
			spec->field_width = -spec->field_width;
			spec->flags |= LEFT;
		}
		spec->type = FORMAT_TYPE_NONE;
		goto precision;
	}

	/* We finished early by reading the percision */
	if(spec->type == FORMAT_TYPE_PRECISION) {
		if(spec->precision < 0)
			spec->precision = 0;

		spec->type = FORMAT_TYPE_NONE;
		goto qualifier;
	}

	/* By default */
	spec->type = FORMAT_TYPE_NONE;

	for(; *fmt ; ++fmt) {
		if(*fmt == '%')
			break;
	}

	/* Return the current non-format string */
	if(fmt != start || *fmt)
		return fmt - start;

	/* Process flags */
	spec->flags = 0;

	while(1) { /* this also skip first % */
		bool found = true;

		++fmt;

		switch(*fmt) {
			case '-': spec->flags |= LEFT; break;
			case '+': spec->flags |= PLUS; break;
			case ' ': spec->flags |= SPACE; break;
			case '#': spec->flags |= SPECIAL; break;
			case '0': spec->flags |= ZEROPAD; break;
			default : found = false;
		}
	/* get field width */
		spec->field_width = -1;

		if(isdigit(*fmt))
			spec->field_width = skip_atoi(&fmt);
		else if(*fmt == '*') {
			/* It's the next argument */
			spec->type = FORMAT_TYPE_WIDTH;
			return ++fmt - start;
		}
precision:
		/* get the precision */
		spec->precision = -1;
		if(*fmt == '.') {
			++fmt;
			if(isdigit(*fmt)) {
				spec->precision = skip_atoi(&fmt);
				if(spec->precision < 0)
					spec->precision = 0;
			} else if(*fmt == '*') {
				/* It's the next argument */
				spec->type = FORMAT_TYPE_PRECISION;
				return ++fmt - start;
			}
		}
qualifier:
		/* get the conversion qualifier */
		spec->qualifier = -1;
		if(*fmt == 'h' || TOLOWER(*fmt) == 'l' ||
				TOLOWER(*fmt) == 'Z' || *fmt == 't') {
			spec->qualifier = *fmt++;
			if(unlikely(spec->qualifier == *fmt)) {
				if(spec->qualifier == 'l') {
					spec->qualifier = 'L';
					++fmt;
				} else if(spec->qualifier == 'h') {
					spec->qualifier = 'H';
					++fmt;
				}
			}
		}
		/* default base */
		spec->base = 10;
		switch(*fmt) {
			case 'c':
				spec->type = FORMAT_TYPE_CHAR;
				return ++fmt - start;
			
			case 's':
				spec->type = FORMAT_TYPE_STR;
				return ++fmt - start;

			case 'p':
				spec->type = FORMAT_TYPE_PTR;
				return fmt - start;

			case 'n':
				spec->type = FORMAT_TYPE_NRCHARS;
				return ++fmt - start;
			
			case '%':
				spec->type = FORMAT_TYPE_PERCENT_CHAR;
				return ++fmt - start;

			/* Interger number formats - set up the flags and break */
			case 'o':
				spec->base = 8;
				break;

			case 'x':
				spec->flags |= SMALL;

			case 'X':
				spec->base = 16;
				break;

			case 'd':
			case 'i':
				spec->flags |= SIGN;
			case 'u':
				break;

			default:
				spec->type = FORMAT_TYPE_INVALID;
				return fmt - start;
		}

		if(spec->qualifier == 'L')
			spec->type = FORMAT_TYPE_LONG_LONG;
		else if(spec->qualifier == 'l')
			if(spec->flags & SIGN)
				spec->type = FORMAT_TYPE_LONG;
			else
				spec->type = FORMAT_TYPE_ULONG;
		else if(TOLOWER(spec->qualifier) == 'z') 
			spec->type = FORMAT_TYPE_SIZE_T;
		else if(spec->qualifier == 't')
			spec->type = FORMAT_TYPE_PTRDIFF;
		else if(spec->qualifier == 'H')
			if(spec->flags & SIGN)
				spec->type = FORMAT_TYPE_BYTE;
			else
				spec->type = FORMAT_TYPE_UBYTE;
		else if(spec->qualifier == 'h')
			if(spec->flags & SIGN)
				spec->type = FORMAT_TYPE_SHORT;
			else
				spec->type = FORMAT_TYPE_USHORT;
		else
			if(spec->flags & SIGN)
				spec->type = FORMAT_TYPE_INT;
			else
				spec->type = FORMAT_TYPE_UINT;
	}
	return ++fmt - start;
}

/*
 * simple_strtoull - conver a string to an unsigned long long
 * @cp: The start of the string.
 * @endp: A pointer to the end of the parsed string will be placed here.
 * @base: The number base to use.
 */
static unsigned long long simple_strtoull(const char *cp,
		char **endp,unsigned int base)
{
	unsigned long long result = 0;

	if(!base)
		base = simple_guess_base(cp);

	if(base == 16 && cp[0] == '0' && TOLOWER(cp[1]) == 'X')
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

/*
 * simple_strtoul - convert a string to an unsigned long
 * @cp: The start of the string.
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use.
 */
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	return simple_strtoull(cp,endp,base);
}

/*
 * simple_strtol - convert a string to a signed long
 * @cp: The start of the string.
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use.
 */
long simple_strtol(const char *cp,char **endp,unsigned int base)
{
	if(*cp == '-')
		return -simple_strtoul(cp + 1, endp , base);

	return simple_strtoul(cp,endp,base);
}

/*
 * Format a string and place it in a buffer.
 */
int vsnprintf(char *buf,size_t size,const char *fmt,va_list args)
{
	unsigned long long num;
	char *str,*end;
	struct printf_spec spec = {0};

	/* 
	 * Reject out-of-range values early.Large positive sizes are 
	 * used for unknow buffer sizes.
	 */
	if(WARN_ON_ONCE((int)size < 0))
		return 0;

	str = buf;
	end = buf + size;

	/* Make sure end is always >= buf */
	if(end < buf) {
		end = ((void *)-1);
		size = end - buf;
	}

	while(*fmt) {
		const char *old_fmt = fmt;
		int read = format_decode(fmt,&spec);

		fmt += read;

		switch(spec.type) {
			case FORMAT_TYPE_NONE: {
				int copy = read;
				if(str < end) {
					if(copy > end - str)
						copy = end - str;
					memcpy(str,old_fmt,copy);
				}
				str += read;
				break;
			}
			case FORMAT_TYPE_WIDTH:
				spec.field_width = va_arg(args,int);
				break;
			case FORMAT_TYPE_PRECISION:
				spec.precision = va_arg(args,int);
				break;
			case FORMAT_TYPE_CHAR: {
				char c;

				if(!(spec.flags & LEFT)) {
					while(--spec.field_width > 0) {
						if(str < end)
							*str = ' ';
						++str;
					}
				}
				c = (unsigned char)va_arg(args,int);
				if(str < end)
					*str = c;
				++str;
				while(--spec.field_width > 0) {
					if(str < end)
						*str = ' ';
					++str;
				}
				break;				   
			}
			case FORMAT_TYPE_STR:
				str = string(str,end,va_arg(args,char *),spec);
				break;
			case FORMAT_TYPE_PTR:
				str = pointer(fmt + 1,str,end,va_arg(args,void *),
						spec);
				while(isalnum(*fmt))
					fmt++;
				break;
			case FORMAT_TYPE_PERCENT_CHAR:
				if(str < end)
					*str = '%';
				++str;
				break;
			case FORMAT_TYPE_INVALID:
				if(str < end)
					*str = '%';
				++str;
				break;
			case FORMAT_TYPE_NRCHARS: {
				u8 qualifier = spec.qualifier;
				
				if(qualifier == 'l') {
					long *ip = va_arg(args,long *);
					*ip = (str - buf);
				} else if(TOLOWER(qualifier) == 'z') {
					size_t *ip = va_arg(args,size_t *);
					*ip = (str - buf);
				} else {
					int *ip = va_arg(args,int *);
					*ip = (str - buf);
				}
				break;
			  }
			default:
					switch(spec.type) {
					case FORMAT_TYPE_LONG_LONG:
						num = va_arg(args,long long);
						break;
					case FORMAT_TYPE_ULONG:
						num = va_arg(args,unsigned long);
						break;
					case FORMAT_TYPE_LONG:
						num = va_arg(args,long);
						break;
					case FORMAT_TYPE_SIZE_T:
						num = va_arg(args,size_t);
						break;
					case FORMAT_TYPE_PTRDIFF:
						num = va_arg(args,ptrdiff_t);
						break;
					case FORMAT_TYPE_UBYTE:
						num = (unsigned char)va_arg(args,int);
						break;
					case FORMAT_TYPE_USHORT:
						num = (unsigned short)va_arg(args,int);
						break;
					case FORMAT_TYPE_SHORT:
						num = (short)va_arg(args,int);
						break;
					case FORMAT_TYPE_INT:
						num = (int)va_arg(args,int);
						break;
					default:
						num = va_arg(args,unsigned int);
					}

					str = number(str,end,num,spec);
			}
	}
	if(size > 0) {
		if(str < end)
			*str = '\0';
		else
			end[-1] = '\0';
	}

	/* The trailing null byte doesn't count towards the total */
	return str - buf;
}

/**
 * vscnprintf - Format a string and place it in a buffer
 * buf: The buffer to place the result into
 * @size: The size of the buffer,including the trailing null space
 * @fmt: The format string the use
 * args: Arguments for the format string
 *
 * The return value is the number of characters which have been written into
 * the @buf not including the trailing '\0'.If @size is == 0 the function.
 * returns 0.
 *
 * Call this functiion if you are already dealing with a va_list.
 * Ypu probably want scnprintf() instead.
 *
 * See the vsnprintf() documentation for format string extensions over C99.
 */
int vscnprintf(char *buf,size_t size,const char *fmt,va_list args)
{
	int i;

	i = vsnprintf(buf,size,fmt,args);

	if(likely(i < size))
		return i;
	if(size != 0)
		return size - 1;
	return 0;
}


/**
 * scnprintf - Format a string and place it in a buffer
 * @buf: The buffer to place the result into
 * @size: The size of the buffer,including the trailing null space
 * @fmt: The format string to use
 * @...: Argruments for the format string
 *
 * The return value is the number of characters written into @buf not including
 * the trailing '\0'.If @size is == 0 the function return 0.
 */

int scnprintf(char *buf,size_t size,const char *fmt,...)
{
	va_list args;
	int i;

	va_start(args,fmt);
	i = vscnprintf(buf,size,fmt,args);
	va_end(args);

	return i;
}

