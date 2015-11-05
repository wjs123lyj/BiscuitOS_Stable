#include "../../include/linux/mm_type.h"
#include "../../include/linux/sched.h"
#include "../../include/linux/config.h"

#ifdef CONFIG_MM_OWNER
void mm_init_owner(struct mm_struct *mm,struct task_struct *p)
{
	mm->owner = p;
}
#endif
