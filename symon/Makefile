# $Id: Makefile,v 1.26 2002/09/14 15:49:39 dijkstra Exp $
.include "../Makefile.inc"

LIBS=	-L../lib -lsymon
SRCS=	symon.c sm_cpu.c sm_mem.c sm_if.c sm_io.c sm_pf.c readconf.c symonnet.c 
OBJS+=	${SRCS:R:S/$/.o/g}
CFLAGS+=-DNET_INET6 -I../lib

all: symon symon.cat8

symon: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS}
.ifndef DEBUG
	${STRIP} $@
.endif

clean:
	rm -f symon symon.cat8 ${OBJS}

install: symon symon.cat8 symon.conf
	${INSTALL} -c -m 555  -g wheel -o root symon      ${PREFIX}/${BINDIR}
	${INSTALL} -c -m 444  -g wheel -o root symon.cat8 ${PREFIX}/${MANDIR}/cat8/symon.0
	${INSTALL} -d -m 555  -g bin   -o root ${PREFIX}/${SHRDIR}
	${INSTALL} -c -m 444  -g bin   -o root symon.conf ${PREFIX}/${SHRDIR}

