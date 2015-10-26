CC=gcc
FLAGS=-g -pg
DEBUG_FLAGS=-C -E -g 
SRCS = main.c \
	   ../lib/mm/memblock.c \
	   ../lib/bitmap.c \
	   ../lib/list.c \
	   ../lib/mm/mmu.c \
	   ../lib/debug.c \
	   ../lib/mm/init-mm.c \
	   ../lib/boot_arch.c \
	   ../lib/mm/init.c \
	   ../lib/mm/pgtable.c

INC= -I./
TARGET=align
OBJS = ${SRCS:.c=.o}
LOG_FILE=debug.log
${TARGET}:
	${CC} ${SRCS} ${FLAGS} -o ${TARGET}
	ctags -R
#	./${TARGET}
	./${TARGET} > ${LOG_FILE}
	vim ${LOG_FILE}
debug:
	${CC} ${SRCS} ${DEBUG_FLAGS}
memblock.o:
	${CC} ../lib/mm/memblock.c ${FLAGS} -c
clean:
	rm -rf *.o *.out
	ctags -R
	rm ${TARGET}
	rm ${LOG_FILE}
