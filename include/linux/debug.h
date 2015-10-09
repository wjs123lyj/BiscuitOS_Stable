#ifndef _DEBUG_H_
#define _DEBUG_H_
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

#ifdef DEBUG
#define mm_debug printf
#else
#define mm_debug
#endif

#define mm_err printf

#define MM_NOREGION 22 /* No match region */
#define MM_NOEXPAND 23 /* Can't expand array */

#endif
