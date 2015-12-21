#include "../../include/linux/kernel.h"
#include "../../include/linux/mm_types.h"

#ifdef CONFIG_MM_OWNER
void mm_init_owner(struct mm_struct *mm,struct task_struct *p)
{
	mm->owner = p;
}
#endif
