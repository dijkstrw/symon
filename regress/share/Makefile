# $Id: Makefile,v 1.4 2004/08/08 19:56:03 dijkstra Exp $
.include "../../Makefile.inc"

SRCS=	testshare.c ../../symux/share.c
OBJS=	${SRCS:R:S/$/.o/g}
LIBS=  -L../../lib -lsymon
CFLAGS+=-I../../lib -I../../platform/${OS} -I../../symux -DTESTSHARE

all: testshare

testshare: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS}
.ifndef DEBUG
	${STRIP} $@
.endif
