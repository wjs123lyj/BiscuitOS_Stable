#include "linux/kernel.h"

int __init reboot_setup(char *str)
{
#ifdef COFNIG_NO_USE

#endif
	return 0;
}
