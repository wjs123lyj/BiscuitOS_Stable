#ifndef _PRINTK_H_
#define _PRINTK_H_

#define KERN_EMERG "<0>"     /* system is unusable */
#define KERN_ALERT "<1>"     /* action must be taken ummediately */
#define KERN_CRIT  "<2>"     /* critial conditions */
#define KERN_ERR   "<3>"     /* error conditions */
#define KERN_WARNING "<4>"   /* warning conditions */
#define KERN_NOTICE  "<5>"   /* normal but significant condition */
#define KERN_INFO    "<6>"   /* informational */
#define KERN_DEBUG   "<7>"   /* debug-level messages */

static inline int printk_ratelimit(void)
{
	return 0;
}

#endif
