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
/*
 * Test bit
 */
int test_bit(int nr,unsigned int *byte)
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
	if(mask & *f_bytes)
		return 1;
	else
		return 0;
}
/*
 * find the first zero bit.
 */
int find_next_zero_bit(unsigned int *byte,unsigned long size,unsigned long offset)
{
	unsigned int *f_bytes;
	unsigned int mask;
	int i,j,n;

	/* Index of byte */
	n = offset >> 5;
	/* Offset of byte */
	f_bytes = byte + n;
	/* Index of bit */
	size >>= 5;

	for(i = 0; i <= size ; i++)
	{
		mask = f_bytes[i];
		for(j = 0 ; j < 32 ; j++)
		{
			if(((mask >> j) & 0x1) == 0)
			{
				return offset + i * 32 + j;
			}
		}
	}
	return -1;
}
