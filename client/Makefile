# $Id: Makefile,v 1.3 2008/01/30 13:19:40 dijkstra Exp $
OS!=uname -s
.include "../platform/${OS}/Makefile.inc"
.include "../Makefile.inc"

all: SymuxClient.cat3p

clean:
	rm -f SymuxClient.cat3p

install: SymuxClient.pm SymuxClient.cat3p getsymonitem.pl
	${INSTALL} -d -m 555 -g ${INSTALLGROUPDIR} -o ${INSTALLUSER} ${PREFIX}/${CLIENTDIR}
	${INSTALL} -c -m 555 -g ${INSTALLGROUPFILE} -o ${INSTALLUSER} getsymonitem.pl      ${PREFIX}/${CLIENTDIR}/
	${INSTALL} -c -m 444 -g ${INSTALLGROUPFILE} -o ${INSTALLUSER} SymuxClient.pm       ${PREFIX}/${CLIENTDIR}/
	${INSTALL} -c -m 444 -g ${INSTALLGROUPFILE} -o ${INSTALLUSER} SymuxClient.cat3p    ${PREFIX}/${CLIENTDIR}/SymuxClient.0
