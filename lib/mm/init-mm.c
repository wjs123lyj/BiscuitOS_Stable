#include "../../include/linux/kernel.h"
#include "../../include/linux/highmem.h"

struct mm_struct init_mm = {
	.pgd = (pgd_t *)swapper_pg_dir,
};
