# $Id: Makefile,v 1.9 2003/12/20 16:30:44 dijkstra Exp $
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
