# $Id: Makefile,v 1.24 2002/08/31 15:00:25 dijkstra Exp $
.include "../Makefile.inc"

LIBS=	-L../lib -lmon
SRCS=	mon.c cpu.c mem.c if.c pf.c readconf.c monnet.c io.c
.ifdef KVM
LIBS+=	-lkvm
.endif
OBJS+=	${SRCS:R:S/$/.o/g}
CFLAGS+=-DNET_INET6 -I../lib

all: mon mon.cat8

mon: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS}
.ifndef DEBUG
	${STRIP} $@
.endif

clean:
	rm -f mon mon.cat8 ${OBJS}

install: mon mon.cat8 mon.conf
.ifdef KVM
	${INSTALL} -c -m 2555 -g kmem  -o root mon      ${PREFIX}/${BINDIR}
.else
	${INSTALL} -c -m 555  -g wheel -o root mon      ${PREFIX}/${BINDIR}
.endif
	${INSTALL} -c -m 444  -g wheel -o root mon.cat8 ${PREFIX}/${MANDIR}/cat8/mon.0
	${INSTALL} -d -m 555  -g bin   -o root ${PREFIX}/${SHRDIR}
	${INSTALL} -c -m 444  -g bin   -o root mon.conf ${PREFIX}/${SHRDIR}

