#ifndef _CURRENT_H_
#define _CURRENT_H_

#include "../linux/thread_info.h"

#define get_current() (current_thread_info()->task)
#define current get_current()

#endif
