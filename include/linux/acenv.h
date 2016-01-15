#ifndef _ACENV_H_
#define _ACENV_H_ 1

typedef char *va_list;

/*
 * Storage alignment properties
 */
#define _AUPBND  (sizeof(signed int) - 1)
#define _ADNBND  (sizeof(signed int) - 1)

#define _bnd(X,bnd)       (((sizeof(X)) + (bnd)) & (~(bnd)))
#define va_arg(ap,T)      (*(T *)(((ap) += (_bnd(T,_AUPBND))) - \
							(_bnd(T,_ADNBND))))
#define va_end(ap)        (void)0
#define va_start(ap,A)    (void)((ap) = (((char *)&(A)) + (_bnd(A,_AUPBND))))

#endif
