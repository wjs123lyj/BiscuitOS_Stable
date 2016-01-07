#include "../../include/linux/kernel.h"
#include "../../include/linux/setup.h"


#define BOOT_PARAMS_SIZE   1536
static char __initdata atags_copy[BOOT_PARAMS_SIZE];

void __init save_atags(const struct tag *tags)
{
	memcpy(atags_copy,tags,sizeof(atags_copy));
}
