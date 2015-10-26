#include "../../include/linux/debug.h"
#include "../../include/linux/pgtable.h"
#include "../../include/linux/init_mm.h"
#include "../../include/linux/mm_type.h"
#include "../../include/linux/page.h"

inline pmd_t *pmd_off_k(unsigned int virt)
{
	return pmd_offset(pgd_offset_k(virt),virt);
}
