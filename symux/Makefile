# $Id: Makefile,v 1.14 2002/09/14 15:54:55 dijkstra Exp $
.include "../Makefile.inc"

SRCS=	symux.c readconf.c symuxnet.c share.c
OBJS+=	${SRCS:R:S/$/.o/g}
LIBS=  -L../lib -L/usr/local/lib -lrrd -lsymon
CFLAGS+=-I../lib -I/usr/local/include

all: symux symux.cat8

symux: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS}
.ifndef DEBUG
	${STRIP} $@
.endif

clean:
	rm -f symux symux.cat8 ${OBJS}

install: symux symux.cat8 c_smrrds.sh symux.conf
	${INSTALL} -c -m 555 -g bin   -o root symux      ${PREFIX}/${BINDIR}
	${INSTALL} -c -m 444 -g wheel -o root symux.cat8 ${PREFIX}/${MANDIR}/cat8/symux.0
	${INSTALL} -d -m 555 -g bin   -o root ${PREFIX}/${SHRDIR}
	${INSTALL} -c -m 544 -g bin   -o root c_smrrds.sh  ${PREFIX}/${SHRDIR}
	${INSTALL} -c -m 444 -g bin   -o root symux.conf   ${PREFIX}/${SHRDIR}

cleanipc:
	ipcs | egrep "^m" | awk '{print $$2}' | xargs -n1 ipcrm -m &
	ipcs | egrep "^s" | awk '{print $$2}' | xargs -n1 ipcrm -s 

