#include "linux/kernel.h"
#include "linux/bitops.h"

static unsigned long index_bits_to_maxindex[BITS_PER_LONG];

void __init prio_tree_init(void)
{
	unsigned int i;

	for(i = 0 ; i < ARRAY_SIZE(index_bits_to_maxindex) - 1 ; i++)
		index_bits_to_maxindex[i] = (1UL << (i + 1)) - 1;
	index_bits_to_maxindex[ARRAY_SIZE(index_bits_to_maxindex) - 1] = ~0UL;
}
