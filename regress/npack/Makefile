# $Id: Makefile,v 1.4 2007/01/20 12:52:48 dijkstra Exp $
.include "../../Makefile.inc"
.include "../../platform/${OS}/Makefile.inc"

LIBS= -L../../lib -lsymon
SRCS= npack.c
OBJS+= ${SRCS:R:S/$/.o/g}
CFLAGS+= -I../../lib -I../../platform/${OS} -I.

all: npack

npack: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS}
.ifndef DEBUG
	${STRIP} $@
.endif

../../lib/libsymon.a:
	cd ../../lib; make libsymon.a

clean:
	rm -f ${OBJS} npack npack.core

