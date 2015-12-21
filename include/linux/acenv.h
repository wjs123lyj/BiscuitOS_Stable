#ifndef _ACENV_H_
#define _ACENV_H_ 1

typedef char *va_list;

#define _bnd(X,bnd)       (((sizeof(X)) + (bnd)) & (~(bnd)))
#define va_arg(ap,T)      (*(T)(((ap) += (__bnd(T,_AUPBND))) - \
							(_bnd(T,_ADNBND))))
#define va_end(ap)        (void)0
#define va_start(ap,A)    (void)((ap) = (((char *)&(A)) + (_bnd(A,_AUPBND))))

#endif
