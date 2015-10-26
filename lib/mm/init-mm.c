#include "../../include/linux/mm_type.h"
#include "../../include/linux/highmem.h"
#include "../../include/linux/init_mm.h"

struct mm_struct init_mm = {
	.pgd = swapper_pg_dir,
};
