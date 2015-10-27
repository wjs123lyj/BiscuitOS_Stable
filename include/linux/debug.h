#ifndef _DEBUG_H_
#define _DEBUG_H_
#include "types.h"
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

#ifdef DEBUG
#define mm_debug printf
#else
#define mm_debug
#endif

#define mm_err printf
#define panic printf

#define BUG_ON(x) \
	if(!!x) \
		mm_err("BUG_ON %s\n",__FUNCTION__); \
	else \
		;
#define WARN_ON(x) (x)

extern void B_show(char *s);
extern void R_show(char *s);
extern void BK_show(char *s);
extern void M_show(phys_addr_t start,phys_addr_t end);
#endif
