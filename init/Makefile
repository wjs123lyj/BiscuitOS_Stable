CC=gcc
FLAGS=-g 
SRCS = main.c 

INC= -I./
TARGET=align
OBJS = ${SRCS:.c=.o}
${TARGET}:memblock.o
	${CC} ${SRCS} ${FLAGS} -o ${TARGET}
memblock.o:
	${CC} ../lib/mm/memblock.c ${FLAGS} -c
clean:
	rm -rf *.o a
	ctags -R
