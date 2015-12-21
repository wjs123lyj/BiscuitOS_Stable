#ifndef _DEBUG_H_
#define _DEBUG_H_   1

#define DEBUG

#ifdef DEBUG
#define mm_debug printf
#else
#define mm_debug
#endif

#define bdebug printf
#define mm_err printf
#define panic printf
#define mm_warn printf

#if 0
#define BUG_ON(x) ({ \
	if((!!(x))) \
		mm_err("BUG_ON %s:%d\n",__FUNCTION__,__LINE__); \
	else \
		; \
		})
#endif
#define BUG_ON(x)  (x)
#define WARN_ON(x) BUG_ON(x)
#define VM_BUG_ON(x) BUG_ON(x)
#define BUILD_BUG_ON(x) BUG_ON(x)
#define BUILD_ON(x)     BUG_ON(x)
#define VIRTUAL_BUG_ON(x)   BUG_ON(x)
#define WARN_ON_ONCE(x)   BUG_ON(x)


#define BUG() \
	mm_err("ERR:%s %d\n",__FUNCTION__,__LINE__)

#endif
