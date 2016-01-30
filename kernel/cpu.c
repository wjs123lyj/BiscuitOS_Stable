#include "linux/kernel.h"
#include "linux/bitops.h"
#include "linux/threads.h"
#include "linux/cpumask.h"

static DECLARE_BITMAP(cpu_possible_bits,CONFIG_NR_CPUS) __read_mostly
	= CPU_BITS_ALL;

const struct cpumask *const cpu_possible_mask = to_cpumask(cpu_possible_bits);
