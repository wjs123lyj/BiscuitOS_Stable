#include "../../include/linux/mm_type.h"
#include "../../include/linux/highmem.h"
#include "../../include/linux/init_mm.h"
#include "../../include/linux/pgtable.h"

struct mm_struct init_mm = {
	.pgd = (pgd_t *)swapper_pg_dir,
};
