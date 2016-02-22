#include "linux/kernel.h"
#include "linux/page.h"
#include "linux/pgtable.h"
#include "linux/memory.h"

static struct page *pcpu_addr_to_page(void *addr)
{
	return virt_to_page(addr);
}
