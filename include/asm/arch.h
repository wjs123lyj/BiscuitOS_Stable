#ifndef _ARCH_H_
#define _ARCH_H_

struct machine_desc {
	unsigned int nr;  /* architecture number */
	const char *name; /* architecture name */
	unsigned long boot_params; /* tagged list */
	unsigned int nr_irqs;      /* number of IRQs */
	unsigned int video_start;  /* start of video RAM */
	unsigned int video_end;    /* end of video RAM */
	unsigned int reserve_lp0:1;  /* never has lp0 */
	unsigned int reserve_lp1:1;  /* never has lp1 */
	unsigned int reserve_lp2:1;  /* never has lp2 */
};

#endif
