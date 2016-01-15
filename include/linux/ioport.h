#ifndef _IOPORT_H_
#define _IOPORT_H_

/*
 * Resource are tree-like,allowing
 * nesting etc...
 */
struct resource {
	resource_size_t start;
	resource_size_t end;
	const char *name;
	unsigned long flags;
	struct resource *parent,*sibling,*child;
};

/*
 * IO resource have these defined flags.
 */
#define IORESOURCE_BITS        0x000000ff /* Bus-specific bits */
#define IORESOURCE_TYPE_BITS   0x00001f00 /* Resource type */
#define IORESOURCE_IO          0x00000100
#define IORESOURCE_MEM         0x00000200
#define IORESOURCE_IRQ         0x00000400
#define IORESOURCE_DMA         0x00000800
#define IORESOURCE_BUS         0x00001000

#define IORESOURCE_BUSY        0x80000000

#define IORESOURCE_PREFETCH    0x00002000

#define IORESOURCE_MEM_64      0x00100000
#define IORESOURCE_WINDOW      0x00200000
#define IORESOURCE_MUXED       0x00400000

#define IORESOURCE_DISABLED    0x10000000


#endif
