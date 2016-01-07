#ifndef _FS_H_
#define _FS_H_   1

/*
 * These ara the fs-independend mount-flags:up to 32 flags are supported.
 */
#define MS_RDONLY   1 /* Mount read-only */
#define MS_NOSUID   2 /* Ignore suid and sgid bits */
#define MS_NODEV    4 /* Disallow access to device specical files */
#define MS_NOEXEC   8 /* Disallow program execution */

#define MS_SILENT   32768

struct address_space {
	unsigned long nrpages;  /* number of total pages */
};

#endif
