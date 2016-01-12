CC=gcc
FLAGS=-g -pg
DEBUG_FLAGS=-C -E -g -Wfatal-errors 
SRCS = main.c \
	   ../lib/mm/memblock.c \
	   ../lib/bitmap.c \
	   ../lib/mm/mmu.c \
	   ../lib/debug.c \
	   ../lib/mm/init-mm.c \
	   ../lib/boot_arch.c \
	   ../lib/kernel/init.c \
	   ../lib/kernel/wait.c \
	   ../lib/kernel/sort.c \
	   ../lib/mm/tlb.c \
	   ../lib/mm/traps.c \
	   ../lib/mm/highmem.c \
	   ../lib/mm/memory.c \
	   ../lib/mm/page_alloc.c \
	   ../lib/mm/bootmem.c \
	   ../lib/mm/fork.c \
	   ../lib/kernel/setup.c \
	   ../lib/kernel/cmdline.c \
       ../lib/mm/mm_init.c  \
	   ../lib/mm/mmzone.c  \
	   ../lib/mm/vmscan.c \
	   ../lib/kernel/init_task.c \
	   ../lib/mm/nommu.c  \
	   ../lib/kernel/resource.c \
	   ../lib/kernel/atags.c \
	   ../lib/kernel/reboot.c \
	   ../lib/kernel/string.c \
	   ../lib/kernel/mach-6410.c \
	   ../lib/kernel/compat.c \
	   ../lib/kernel/ctype.c \
	   ../lib/kernel/params.c \
	   ../lib/mm/debug-pagealloc.c \
	   ../lib/mm/memcontrol.c     \
	   ../lib/mm/mmap.c    \
	   ../lib/mm/page_cgroup.c  \
       ../lib/mm/pte.c   \
	   ../lib/mm/vmstat.c \
	   ../lib/mm/swap.c \
	   ../lib/mm/sched.c  \
	   ../lib/mm/slub.c   \
	   ../lib/mm/swap_state.c \
	   ../lib/mm/rbtree.c  \
	   ../lib/mm/radix-tree.c  \
	   ../lib/mm/percpu.c   \
	   ../lib/mm/kmemcheck.c   \
	   ../lib/mm/vmalloc.c  \
	   ../lib/kernel/head.c  \
	   ../lib/kernel/do_mounts.c  \
	   ../lib/register_simulate.c  \
	   ../lib/uboot.c  \
	   ../lib/mm/test.c


INC= -I./
TARGET=align
OBJS = ${SRCS:.c=.o}
LOG_FILE=debug.log
${TARGET}:
	${CC} ${SRCS} ${FLAGS} -o ${TARGET} 2>&1|tee compiler.log
	ctags -R
	./${TARGET}
#	./${TARGET} > ${LOG_FILE}
#	vim ${LOG_FILE}
debug:
	${CC} ${SRCS} ${DEBUG_FLAGS}
memblock.o:
	${CC} ../lib/mm/memblock.c ${FLAGS} -c
clean:
	rm -rf *.o *.out
	ctags -R
	rm ${TARGET}
	rm ${LOG_FILE}
