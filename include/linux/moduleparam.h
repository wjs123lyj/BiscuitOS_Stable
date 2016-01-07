#ifndef _MODULEPARAM_H_
#define _MODULEPARAM_H_

struct kernel_param;

struct kernel_param_ops {
	/* Return 0,or - errno,arg is in kp->arg. */
	int (*set)(const char *val,const struct kernel_param *kp);
	/* Return length written or -errno.Buffer is 4k */
	int (*get)(char *buffer,const struct kernel_param *kp);
	/* Optional function to keep kp->arg when module unloaded. */
	void (*free)(void *arg);
};

/* Flags bit for kernel_param.flags */
#define KPARAM_ISBOOL    2

struct kernel_param {
	const char *name;
	const struct kernel_param_ops *ops;
	u16 perm;
	u16 flags;
	union {
		void *arg;
		const struct kparam_string *str;
		const struct kparam_array *arr;
	};
};

/* Special one for strings we want to copy into */
struct kparam_string 
{
	unsigned int maxlen;
	char *string;
};

/* Special one for arrays */
struct kparam_array 
{
	unsigned int max;
	unsigned int *num;
	const struct kernel_param_ops *ops;
	unsigned int elemsize;
	void  *elem;
};



#endif
