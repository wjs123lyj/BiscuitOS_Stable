#include "linux/kernel.h"
#include "linux/sched.h"
#include "linux/thread_info.h"

struct task_struct current_p;
struct thread_info current_thread = {
	.task = &current_p,
};
