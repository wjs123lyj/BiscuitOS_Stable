CC=gcc
FLAGS=-g -pg 
SRCS = main.c \
	   ../lib/mm/memblock.c

INC= -I./
TARGET=align
OBJS = ${SRCS:.c=.o}
LOG_FILE=debug.log
${TARGET}:
	${CC} ${SRCS} ${FLAGS} -o ${TARGET}
	ctags -R
	./${TARGET} > ${LOG_FILE}
	vim ${LOG_FILE}
memblock.o:
	${CC} ../lib/mm/memblock.c ${FLAGS} -c
clean:
	rm -rf *.o *.out
	ctags -R
	rm ${TARGET}
	rm ${LOG_FILE}
