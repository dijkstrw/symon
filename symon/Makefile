# $Id: Makefile,v 1.19 2002/07/20 14:28:30 dijkstra Exp $
.include "../Makefile.inc"

SRCS=	mon.c cpu.c mem.c if.c io.c readconf.c monnet.c
OBJS+=	${SRCS:R:S/$/.o/g}
LIBS=  -lkvm -L../lib -lmon
CFLAGS+=-DMON_KVM -DNET_INET6 -I../lib

all: mon mon.cat8

mon: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}
	${STRIP} $@

clean:
	rm -f mon mon.cat8 ${OBJS}

install: mon mon.cat8
	${INSTALL} -c -m 2555 -g kmem  -o root mon                  ${PREFIX}/${BINDIR}
	${INSTALL} -c -m 444  -g wheel -o root mon.cat8             ${PREFIX}/${MANDIR}/cat8/mon.0
	${INSTALL} -d -m 555  -g bin   -o root                      ${PREFIX}/${SHRDIR}
	${INSTALL} -c -m 444  -g bin   -o root ../examples/mon.conf ${PREFIX}/${SHRDIR}

