# $Id: Makefile,v 1.7 2002/08/29 05:59:34 dijkstra Exp $
.include "../Makefile.inc"

SRCS=   error.c lex.c xmalloc.c net.c data.c
OBJS+=	${SRCS:R:S/$/.o/g}

all: libmon.a

libmon.a: ${OBJS}
	@echo building standard mon library
	@rm -f libmon.a
	@${AR} cq libmon.a `${LORDER} ${OBJS} | tsort -q`
	${RANLIB} libmon.a

clean:
	rm -f libmon.a ${OBJS}

install: libmon.a

