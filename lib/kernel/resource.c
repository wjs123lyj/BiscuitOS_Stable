#include "linux/kernel.h"
#include "asm/errno-base.h"
#include "linux/ioport.h"
#include "asm/io.h"
#include "linux/debug.h"


struct resource ioport_resource = {
	.name   = "PCI IO",
	.start  = 0,
	.end    = IO_SPACE_LIMIT,
	.flags  = IORESOURCE_IO,
};

struct resource iomem_resource = {
	.name   = "PCI mem",
	.start  = 0,
	.end    = -1,
	.flags  = IORESOURCE_MEM,
};

/*
 * Return the conflict entry if you can't request it.
 */
static struct resource * __request_resource(struct resource *root,
		struct resource *new)
{
	resource_size_t start = new->start;
	resource_size_t end   = new->end;
	struct resource *tmp,**p;

	if(end < start)
		return root;
	if(start < root->start)
		return root;
	if(end > root->end)
		return root;
	p = &root->child;
	for(;;) {
		tmp = *p;
		if(!tmp || tmp->start > end) {
			new->sibling = tmp;
			*p = new;
			new->parent = root;
			return NULL;
		}
		p = &tmp->sibling;
		if(tmp->end < start) {
			continue;
		}
		return tmp;
	}
}


/*
 * Request_resource_conflict - request and resource an I/O or memory resource
 * Return 0 for success,conflict resource on error.
 */
struct resource *request_resource_conflict(struct resource *root,struct resource *new)
{
	struct resource *conflict;
	conflict = __request_resource(root,new);
	return conflict;
}
/*
 * Request_resource - request and reserve an I/O or memory resource.
 * Return 0 for success,negative error code on error.
 */
int request_resource(struct resource *root,struct resource *new)
{
	struct resource *conflict;
	conflict = request_resource_conflict(root,new);
	return conflict ? -EBUSY:0;
}

