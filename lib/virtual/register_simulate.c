#include "linux/kernel.h"
#include "asm/system.h"

/*
 * Coprocessor Registion - CP15
 */
unsigned int CP15_C00 = 0; /* ID register */
unsigned int CP15_C01 = 0; /* Cache type register */
unsigned int CP15_C1 = 0;
unsigned int CP15_C2 = 0;
unsigned int CP15_C3 = 0;
unsigned int CP15_C4 = 0;
unsigned int CP15_C5 = 0;
unsigned int CP15_C6 = 0;
/*
 * Register Map on ARM
 */
static u32 r1;
static u32 r2;

/*
 * CR1 register
 */
u32 get_cr1(void)
{
	return r1;
}

void set_cr1(u32 value)
{
	r1 = value;
}
/*
 * CR2 register
 */
u32 get_cr2(void)
{
	return r2;
}
void set_cr2(u32 value)
{
	r2 = value;
}

/*
 * Initialize the register of CP15 during uboot.
 */
void __uboot init_CP15(void)
{
	/* Initialize the C0 register that is ID register. */
	init_CP15_C00();
	/* Initialize the C0 register that is cache type register. */
	init_CP15_C01();
	/* The C1 regions */
	init_CP15_C1();
	/* The C2 regions */

	/* The C3 regions */
}
