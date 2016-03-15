#ifndef _PTRACE_H_
#define _PTRACE_H_

struct pt_regs {
	unsigned long uregs[18];
};

#endif
