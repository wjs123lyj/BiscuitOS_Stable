#include "../../include/linux/kernel.h"
#include "../../include/asm/head.h"
#include "../../include/linux/mm_types.h"

struct mm_struct init_mm = {
	.pgd = swapper_pg_dir,
};
