# $Id: Makefile.inc,v 1.1 2003/10/05 07:52:52 dijkstra Exp $

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