#ifndef _SECTIONS_H_
#define _SECTIONS_H_

extern char __executable_start[];
extern char __etext[], _etext[], etext[];
extern char __edata[], _edata[], edata[];
extern char _end[],end[];

/*
 * function descriptor handing.Override 
 * in asm/sections.h
 */
#define dereference_function_descriptor(p)  (p)

#endif
