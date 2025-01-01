.include "../../Makefile.inc"

SRCS=	testshare.c ../../symux/share.c ../../symux/symuxnet.c
OBJS=	${SRCS:R:S/$/.o/g}
LIBS=  -L../../lib -lsym
CFLAGS+=-I../../lib -I../../platform/${OS} -I../../symux -DTESTSHARE

all: testshare

testshare: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS}
