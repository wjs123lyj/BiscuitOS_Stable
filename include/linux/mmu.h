#ifndef _MMU_H_
#define _MMU_H_

extern void *vmalloc_min;
extern pgprot_t protection_map[16];

extern pgprot_t pgprot_user;
extern pgprot_t pgprot_kernel;


#endif
