#ifndef _CPUMASK_H_
#define _CPUMASK_H_

#define cpu_online_mask 0
#define cpu_possible_mask 0

#define for_each_cpu(cpu,mask)         \
	for((cpu) = 0 ; (cpu) < 1 ; (cpu)++,(void)mask)

#define for_each_online_cpu(cpu)  for_each_cpu((cpu),cpu_online_mask)
#define for_each_possible_cpu(cpu)  for_each_cpu((cpu),cpu_possible_mask)

#define nr_cpu_ids    1
#define num_online_cpus()   1U

#endif
