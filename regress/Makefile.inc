# $Id: Makefile.inc,v 1.2 2003/12/20 16:30:44 dijkstra Exp $

CC=	 cc
CFLAGS+= -g -Wall
OBJS+=   ${SRCS:R:S/$/.o/g}


.SUFFIXES: .c .o

.c.o:
	${CC} ${CFLAGS} -c $<

all:  ${TARGET} ${DEPS}

${TARGET}: ${OBJS} ${DEPS}
	${CC} -o $@ ${OBJS} ${LIBS}

regress:  ${TARGET}
	./${TARGET}

clean:
	rm -f ${TARGET} ${TARGET}.core ${OBJS}