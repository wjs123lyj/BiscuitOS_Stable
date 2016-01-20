#include "linux/debug.h"
#include "linux/bitops.h"

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
 * Set bit
 */
void set_bit(int nr,unsigned int *byte)
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
	/* Set bit */
	*f_bytes |= mask;
}
/*
 * Clear bit
 */
void clear_bit(int nr,unsigned int *byte)
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
	/* Clear bit */
	*f_bytes &= ~mask;
}
/*
 * find the first zero bit.
 * @offset must be small than @size.
 */
int find_next_zero_bit(unsigned int *byte,
		unsigned long size,unsigned long offset)
{
	unsigned int *f_bytes;
	unsigned int mask;
	int i,j,n,m;

	/* Index of byte */
	n = offset / 32;
	offset = offset % 32;
	/* Offset of byte */
	f_bytes = byte + n;
	/* Index of bit */
	size >>= 5;
	m = offset;

	for(i = 0; i <= size ; i++)
	{
		mask = f_bytes[i];
		mask >>= offset;
		for(j = 0 ; j < 32 - offset ; j++)
		{
			if(((mask >> j) & 0x1) == 0)
			{
				return m + i * 32 + j + n * 32;
			}
		}
		offset = 0;
	}
	return -1;
}
/*
 * Find the next set bit in a memory region.
 */
int find_next_bit(unsigned int *addr,unsigned long size,
		unsigned long offset)
{
	unsigned int *f_byte;
	unsigned long i,j,m,n;
	unsigned int mask;

	/* Get offset of Byte */
	n = offset / 32;
	offset = offset % 32;
	/* Get the start address */
	f_byte = addr + n;
	size >>= 5;
	/* Store the offset */
	m = offset;

	for( i = 0 ; i <= size ; i++)
	{
		mask = f_byte[i];
		mask >>= offset;
		for(j = 0 ; j < 32 - offset ; j++)
		{
			if((mask >> j) & 0x01 == 1)
				return i * 32 + n * 32 + m + j;
		}
		offset = 0;
	}
	return -1;
}

