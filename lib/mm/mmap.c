#include "../../include/linux/kernel.h"
#include "../../include/linux/mman.h"
#include "../../include/linux/page.h"

/*
 * The table below defines the page protection levels that we insert into our
 * Linux page table version.These get translated into the best that the
 * architecture can perform.Note that on most ARM haredware:
 * 1) We cann't do execute protection
 * 2) If we could do execute protection,then read is implied
 * 3) write implies read permissions
 */
#define __P000 __PAGE_NONE
#define __P001 __PAGE_READONLY
#define __P010 __PAGE_COPY
#define __P011 __PAGE_COPY
#define __P100 __PAGE_READONLY_EXEC
#define __P101 __PAGE_READONLY_EXEC
#define __P110 __PAGE_COPY_EXEC
#define __P111 __PAGE_COPY_EXEC

#define __S000 __PAGE_NONE
#define __S001 __PAGE_READONLY
#define __S010 __PAGE_SHARED
#define __S011 __PAGE_SHARED
#define __S100 __PAGE_READONLY_EXEC
#define __S101 __PAGE_READONLY_EXEC
#define __S110 __PAGE_SHARED_EXEC
#define __S111 __PAGE_SHARED_EXEC
/*
 * Description of effects of mapping type and prot in current implementation.
 * this is due to the limited x86 page protection hardware.The expected
 * behavior is in parens:
 *
 * map_type prot
 *		PROT_NONE  PROT_READ  PROT_WRITE  PROT_EXEC
 * MAP_SHARED 
 *       r:(no)no   r(yes)yes   r(no)yes   r(no)yes
 *       w:(no)no   w(no)no     w(yes)yes  w(no)no
 *       x:(no)no   x(no)yes    x(no)yes   x(yes)yes
 * MAP_PRIVATE
 *       r:(no)no   r(yes)yes   r(no)yes   r(no)yes
 *       w:(no)no   w(no)no     w(copy)copy w(no)no
 *       x:(no)no   x(no)yes    x(no)yes   x(yes)yes
 */
pgprot_t protection_map[16] = {
	0,0,0,0
#if 0
	__P000,__P001,__P010,__P011,__P100,__P101,__P110,__P111,
	__S000,__S001,__S010,__S011,__S100,__S101,__S110,__S111
#endif
};


int sysctl_overcommit_memory = OVERCOMMIT_GUESS;  /* heuristic overcommit */
