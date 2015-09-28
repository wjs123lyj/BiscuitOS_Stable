CC=gcc
FLAGS=-g -pg 
SRCS = main.c \
	   ../lib/mm/memblock.c

INC= -I./
TARGET=align
OBJS = ${SRCS:.c=.o}
${TARGET}:
	${CC} ${SRCS} ${FLAGS} -o ${TARGET}
	ctags -R
memblock.o:
	${CC} ../lib/mm/memblock.c ${FLAGS} -c
clean:
	rm -rf *.o a
	ctags -R
