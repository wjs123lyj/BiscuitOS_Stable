


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
	FORMAT_TYPE_PERCISION,
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
}
static int skip_atoi(const char **s)
{
	int i = 0;

	while(isdigit(**s))
		i = i * 10 + *((*s)++) - '0';

	return i;
}
/*
 * Show a '%p' thing.
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
		case 'I':
		case 'i':

			switch(fmt[1]) {
				case '6':
					return ip6_addr_string(buf,end,ptr,spec,fmt);
				case '4':
					return ip4_addr_string(buf,end,ptr,spec,fmt);
			}
			break;
		case 'U':
			return uuid_string(buf,end,ptr,spec,fmt);
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
							*str = '';
						++str;
					}
				}
				c = (unsigned char)va_arg(args,int);
				if(str < end)
					*str = c;
				++str;
				while(--spec.field_width > 0) {
					if(str < end)
						*str = '';
					++str;
				}
				break;				   
			}
			case FORMAT_TYPE_STR:
				str = string(str,end,va_arg(args,char *),spec);
				break;
			case FORMAT_TYPE_PTR:
				str = pointer(fmt + 1,str,end,va_arg(arg,void *),
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
						num = (short)va_args(args,int);
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
