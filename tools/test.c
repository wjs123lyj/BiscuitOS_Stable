#include "linux/kernel.h"
#ifdef CONFIG_HEAD_FILE_CHECK
/* Input the header file */
#include "linux/debug.h"
int do_check(void)
{
	/*
	 * If adding the head file and compiling doesn't have
	 * and warning or error,the head file is safe.
	 */
	struct head_check do_check;
	return 0;
}
#endif
