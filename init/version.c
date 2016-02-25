#include "linux/kernel.h"
#include "linux/compile.h"

#define UTS_RELEASE "2.6.38-BuddyARM"

/* FIXED STRINGS! Don't touch! */
const char linux_banner[] = 
	"Linux version" UTS_RELEASE "(" LINUX_COMPILE_BY "@"
	LINUX_COMPILE_HOST ")(" LINUX_COMPILER ")" UTS_VERSION "\n";
