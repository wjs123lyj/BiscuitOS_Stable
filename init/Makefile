CC=gcc
FLAGS=-g -o 
SRCS = main.c \
	   test-add.c \
	   test-sub.c

INC= -I./
TARGET=align
OBJS = ${SRCS:.c=.o}
${TARGET}:
	${CC} ${SRCS} ${FLAGS} ${TARGET}
clean:
	rm -rf *.o a
	ctags -R
