#include <stdio.h>
#include <stdlib.h>

extern char __executable_start[];
extern char __etext[],_etext[],etext[];
extern char __edata[],_edata[],edata[];
extern char _end[],end[];
int main()
{
	printf("EXECULATE: %x\n",_sdata);
	printf("Text section end:%x %x %x\n",__etext,_etext,etext);
	return 0;
}
