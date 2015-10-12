#include "../include/linux/debug.h"

/*
 * Test and set bit.
 */
int test_and_set_bit(int nr,unsigned int *byte)
{
	unsigned int *f_bytes;
	unsigned int mask = 1UL;
	int n;

	/* Index of byte */
	n = nr >> 5;
	f_bytes = byte + n;
	/* Offset of byte */
	n = nr % 32;
	/* Mask */
	mask = mask << n;
	if(*f_bytes & mask)
		return 1;
	else
		*f_bytes |= mask;
	return 0;
}
/*
 * Test and clear bit.
 */
int test_and_clear_bit(int nr,unsigned int *byte)
{
	unsigned int *f_bytes;
	unsigned int mask = 1UL;
	int n;

	/* Index of byte */
	n = nr >> 5;
	f_bytes = byte + n;
	/* Offset of byte */
	n = nr % 32;
	/* Mask */
	mask = ~(mask << n);
	if(*f_bytes & ~mask)
	{
		*f_bytes &= mask;
		return 1;
	}
	else
		*f_bytes &= mask;
	return 0;
}
