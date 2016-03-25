# BiscuitOS_Stable
BiscuitOS is a like Linux system that built in Linux user space.

User can write code and test in BiscuitOS that like run in Linux kernel space.

How to use BiscuitOS:

    download the source code.
    cd /BiscuitOS/
    make

How to write and test your code:

    cd /BiscuitOS/tools/
    create file TestCase_xxx.c
    write your code
    vi Makefile and add "CURRENT_SOURCE += TestCase_xxx.c"
    vi /BiscuitOS/include/linux/testcase.h and add extern function.
    vi /BiscuitOS/init/main.c and find function "kernel_space()",then and you function.
    cd /BiscuitOS and "make"

How to use debug tools

    cd /BiscuitOS/tools
    vi debug.c

Always update....
