#ifndef _MM_TYPE_H_
#define _MM_TYPE_H_

#include "pgtable.h"
/*
 * mm_struct 
 */
struct mm_struct {
	pgd_t *pgd;
};

#endif
