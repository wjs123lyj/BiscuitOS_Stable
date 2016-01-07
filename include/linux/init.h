#ifndef _INIT_H_
#define _INIT_H_

struct obs_kernel_param {
	const char *str;
	int (*setup_func)(char *);
	int early;
};

/*
 * Only for really core code.See noduleparam.h for the normal way.
 *
 * Force the alignment so the compiler doesn't space elements of the 
 * obs_kernel_param "array" too far apart in .init.setup.
 */

#define __setup_param(str,unique_id,fn,early)                   \
	static const char __setup_str_##unique_id[] __initconst    \
			__attribute__((aligned(1))) = str; \
	struct obs_kernel_param __setup_##unique_id          \
            __used __attribute__((section(".init.setup")))           \
            __attribute__((aligned((sizeof(long)))))   \
            = { __setup_str_##unique_id,fn,early }

#define __setup(str,fn)     \
		__setup_param(str,fn,fn,0)


/*
 * NOTE:fn is as per module_param,not __setup! Emits warning if fn
 * return non-zero.
 */
#define early_param(str,fn)     \
	    __setup_param(str,fn,fn,1)
#endif
