#ifndef _SMP_H_
#define _SMP_H_

#define smp_processor_id()   0
#define smp_prepare_boot_cpu() do {} while(0)
#define raw_smp_processor_id()    0

#define on_each_cpu(func,info,wait)      \
 ({                                        \
		func(info);                      \
  })


#endif
