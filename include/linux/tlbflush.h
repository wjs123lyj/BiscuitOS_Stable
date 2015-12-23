#ifndef _TLBFLUSH_H_
#define _TLBFLUSH_H_

static inline void local_flush_tlb_kernel_page(unsigned long kaddr)
{
}
static inline void flush_pmd_entry(pmd_t *pmd)
{
}
#define flush_tlb_kernel_range(s,e) do {} while(0)
#endif
