#include "linux/kernel.h"
#include "linux/irqflags.h"
#include "linux/moduleparam.h"
#include "linux/mutex.h"
#include "asm/errno-base.h"
#include "linux/debug.h"

/* Protect all paramenters,and incidentally kmalloced_param list. */
static int param_lock;

extern char *skip_spaces(const char *str);

/* Actually could be a bool or an int,for historical resons. */
int param_set_bool(const char *val,const struct kernel_param *kp)
{
	bool v;

	/* No equals means 'set'.. */
	if(!val)
		val = "1";

	/* One of = [yYnNo1] */
	switch(val[0]) {
		case 'y': 
		case 'Y':
		case '1':
			v = true;
			break;
		case 'n':
		case 'N':
		case '0':
			v = false;
			break;
		default:
			return -EINVAL;
	}

	if(kp->flags & KPARAM_ISBOOL)
		*(bool *)kp->arg = v;
	else
		*(int *)kp->arg = v;
	return 0;
}

static inline char dash2underscore(char c)
{
	if(c == '-')
		return '_';
	return c;
}

static inline int parameq(const char *input,const char *paramname)
{
	unsigned int i;

	mm_debug("Input name %s paramname %s\n",input,paramname);
	for(i = 0 ; dash2underscore(input[i]) == paramname[i] ; i++)
		if(input[i] == '\0')
			return 1;
	return 0;
}
static int parse_one(char *param,
		char *val,const struct kernel_param *params,
		unsigned num_params,
		int (*handle_unknown)(char *param,char *val))
{
	unsigned int i;
	int err;

	/* Find parameter */
	for(i = 0 ; i < num_params ; i++) {
		if(parameq(param,params[i].name)) {
			/* Noone handled NULL,so do it here. */
			if(!val && params[i].ops->set != param_set_bool)
				return -EINVAL;
			mm_debug("They are equal! Calling %p\n",
					(void *)(params[i].ops->set));
			mutex_lock(&param_lock);
			err = params[i].ops->set(val,&params[i]);
			mutex_unlock(&param_lock);
			return err;
		}
	}
	if(handle_unknown) {
		mm_debug("Inknown argument:calling %p\n",(void *)handle_unknown);
		return handle_unknown(param,val);
	}

	mm_debug("Unknown argument %s\n",param);
	return -ENOENT;
}
/*
 * You can use "around spaces,but can't escape".
 * Hyphens and underscores equivalent in paramenter names.
 */
static char *next_arg(char *args,char **param,char **val)
{
	unsigned int i,equals = 0;
	int in_quote = 0,quoted = 0;
	char *next;

	if(*args == '"') {
		args++;
		in_quote = 1;
		quoted = 1;
		mm_debug("The string is a quote\n");
	}

	for(i = 0 ; args[i]; i++) {
		if(isspace(args[i]) && !in_quote)
			break;
		if(equals == 0) {
			if(args[i] == '=')
				equals = i;
		}
		if(args[i] == '"')
			in_quote = !in_quote;
	}
	
	*param = args;
	if(!equals)
		*val = NULL;
	else {
		args[equals] = '\0';
		*val = args + equals + 1;

		/* Don't include quotes in value */
		if(**val == '"') {
			(*val)++;
			if(args[i - 1] == '"')
				args[i - 1] = '\0';
		}
		if(quoted && args[i - 1] == '"')
			args[i - 1] = '\0';
	}

	if(args[i]) {
		args[i] = '\0';
		next = args + i + 1;
	} else
		next = args + i;

	/* Chew up trailing spaces. */
	return skip_spaces(next);
}

/* Args looks like "foo=bar,bar2 baz=fuz wiz". */
int parse_args(const char *name,char *args,
		const struct kernel_param *params,
		unsigned num,
		int (*unknown)(char *param,char *val))
{
	char *param,*val;

	mm_debug("Parsing ARGS:%s\n",args);
	/* Chew leading spaces */
	args = skip_spaces(args);

	while(*args) {
		int ret;
		int irq_was_disabled;
		
		args = next_arg(args,&param,&val);
		irq_was_disabled = irqs_disabled();
		ret = parse_one(param,val,params,num,unknown);
		if(irq_was_disabled && !irqs_disabled()) {
			mm_warn("parse_args(): option '%s' enable "
					"irq's\n",param);
		}
		switch(ret) {
			case -ENOENT:
				mm_debug("%s:Unknown parameter %s\n",
						name,param);
				return ret;
			case -ENOSPC:
				mm_debug("%s:%s too large for parament %s\n",
						name,val ? :"",param);
				return ret;
			case 0:
				break;
			default:
				mm_debug("%s:%s invalid for parameter %s\n",
						name,val ? : "" , param);
				return ret;
		}
	}
	return 0;
}
