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

#define BUG() do {  \
	mm_debug("BUG:failure at %s:%d %s()!\n",\
			__FILE__,__LINE__,__func__);  \
    } while(0)

#define BUG_ON(condition) do { if((condition)) BUG(); } while(0)

#define WARN_ON(condition)    ({                     \
		int __ret_warn_on = !!(condition);           \
		unlikely(__ret_warn_on);                     \
})

#define VM_BUG_ON(x) WARN_ON(x)
#define BUILD_BUG_ON(x) WARN_ON(x)
#define BUILD_ON(x)     WARN_ON(x)
#define VIRTUAL_BUG_ON(x) WARN_ON(x)
#define WARN_ON_ONCE(x)   WARN_ON(x)




#define line()    \
	mm_debug("[%s %d]\n",__FUNCTION__,__LINE__)

#endif
