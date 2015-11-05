#include "../../include/linux/init_task.h"

struct task_struct init_task = {
	.state = 0xFFFF,
};
