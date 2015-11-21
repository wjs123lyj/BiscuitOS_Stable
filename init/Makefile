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
	   ../lib/mm/init.c \
	   ../lib/mm/pgtable.c \
	   ../lib/mm/tlb.c \
	   ../lib/mm/traps.c \
	   ../lib/mm/highmem.c \
	   ../lib/mm/memory.c \
	   ../lib/mm/page_alloc.c \
	   ../lib/mm/bootmem.c \
	   ../lib/mm/fork.c \
	   ../lib/kernel/setup.c \
       ../lib/mm/mm_init.c  \
	   ../lib/mm/mmzone.c  \
	   ../lib/mm/vmscan.c \
	   ../lib/kernel/init_task.c \
	   ../lib/mm/nommu.c  \
	   ../lib/kernel/resource.c \
	   ../lib/mm/debug-pagealloc.c \
	   ../lib/mm/memcontrol.c     \
	   ../lib/mm/mmap.c    \
	   ../lib/mm/page_cgroup.c  \
       ../lib/mm/pte.c   \
	   ../lib/mm/vmstat.c


INC= -I./
TARGET=align
OBJS = ${SRCS:.c=.o}
LOG_FILE=debug.log
${TARGET}:
	${CC} ${SRCS} ${FLAGS} -o ${TARGET}
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
