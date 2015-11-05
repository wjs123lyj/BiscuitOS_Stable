#ifndef _MAP_H_
#define _MAP_H_

/*
 * Descript of map
 */
struct map_desc {
	unsigned long virtual;
	unsigned long pfn;
	unsigned long length;
	unsigned int type;
};

#endif
