# $Id: Makefile,v 1.10 2002/07/20 14:28:33 dijkstra Exp $
.include "../Makefile.inc"

SRCS=	monmux.c readconf.c muxnet.c share.c
OBJS+=	${SRCS:R:S/$/.o/g}
LIBS=  -L../lib -L/usr/local/lib -lrrd -lmon
CFLAGS+=-I../lib -I/usr/local/include

all: monmux monmux.cat8

monmux: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LIBS}
	${STRIP} $@

clean:
	rm -f monmux monmux.cat8 ${OBJS}

install: monmux monmux.cat8 c_monrrds.sh ../examples/monmux.conf
	${INSTALL} -c -m 555 -g bin   -o root monmux 			${PREFIX}/${BINDIR}
	${INSTALL} -c -m 444 -g wheel -o root monmux.cat8 		${PREFIX}/${MANDIR}/cat8/monmux.0
	${INSTALL} -d -m 555 -g bin   -o root 				${PREFIX}/${SHRDIR}
	${INSTALL} -c -m 444 -g bin   -o root c_monrrds.sh 		${PREFIX}/${SHRDIR}
	${INSTALL} -c -m 444 -g bin   -o root ../examples/monmux.conf 	${PREFIX}/${SHRDIR}
