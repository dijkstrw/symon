# $Id: Makefile,v 1.8 2002/09/14 15:56:18 dijkstra Exp $
.include "../Makefile.inc"

SRCS=   error.c lex.c xmalloc.c net.c data.c
OBJS+=	${SRCS:R:S/$/.o/g}

all: libsymon.a

libsymon.a: ${OBJS}
	@echo building standard symon library
	@rm -f libsymon.a
	@${AR} cq libsymon.a `${LORDER} ${OBJS} | tsort -q`
	${RANLIB} libsymon.a

clean:
	rm -f libsymon.a ${OBJS}

install: libsymon.a

