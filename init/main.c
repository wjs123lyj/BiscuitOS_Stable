#include <stdio.h>
#include <stdlib.h>
#include "test-add.h"
#include "test-sub.h"
#include "buddy.h"

int main()
{
	int a = 3;
	int b = 2;

	printf("a=%d\n",a);
	printf("b=%d\n",b);

	printf("a+b=%d\n",add(a,b));
	printf("a-b=%d\n",sub(a,b));

	printf("Hello World\n");
	buddy_show();
	return 0;
}
