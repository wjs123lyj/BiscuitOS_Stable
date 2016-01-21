#ifndef _MODULE_H_
#define _MODULE_H_

struct kernel_symbol
{
	unsigned long value;
	const char *name;
}

#ifdef CONFIG_MODVERSIONS
/* Mark the CRC weak since genksyms apparently decides not to
 * generate a checksums for some symbols */
#define __CRC_SYMBOL(sym,sec)
#else
#define __CRC_SYMBOL(sym,sec)
#endif

/* Some toolchains use  a '_' prefix for all user symbols. */
#ifdef CONFIG_SYMBOL_PREFIX
#define MODULE_SYMBOL_PREFIX CONFIG_SYMBOL_PREFIX
#else
#define MODULE_SYMBOL_PREFIX ""
#endif


/* For every exported symbol,please a strcut in the __ksymtab section */
#define __EXPORT_SYMBOL(sym,sec)                         \
	extern typeof(sym) sym;                              \
	__CRC_SYMBOL(sym,sec)                                \
	static const char __kstrtab_##sym[]                 \
	__attribute__((section("__ksymtab_strings"),aligned(1)))  \
	= MODULE_SYMBOL_PREFIX #sym;                          \
	static const struct kernel_symbol __ksymtab_##sym     \
	__attribute__((section("__ksymtab" sec),unused))      \
	= {(unsigned long)&sym,__kstrtab_##sym}

#define EXPORT_SYMBOL(sym)            \
	__EXPORT_SYMBOL(sym,"")

#endif
