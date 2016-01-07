#ifndef _KDEV_T_H_
#define _KDEV_T_H_


#define MINORBITS   20

#define MKDEV(ma,mi) (((ma) << MINORBITS) | (mi))

static inline dev_t old_decode_dev(u16 val)
{
	return MKDEV((val >> 8) & 255, val & 255);
}

#endif
