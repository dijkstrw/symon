# $Id: Makefile,v 1.2 2003/12/20 16:30:44 dijkstra Exp $
.include "../Makefile.inc"

all: SymuxClient.cat3p

clean:
	rm -f SymuxClient.cat3p

install: SymuxClient.pm SymuxClient.cat3p getsymonitem.pl
	${INSTALL} -d -m 555 -g www -o www ${PREFIX}/${CLIENTDIR}
	${INSTALL} -c -m 555 -g bin   -o root getsymonitem.pl      ${PREFIX}/${CLIENTDIR}
	${INSTALL} -c -m 444 -g wheel -o root SymuxClient.pm       ${PREFIX}/${CLIENTDIR}
	${INSTALL} -c -m 444 -g wheel -o root SymuxClient.cat3p    ${PREFIX}/${CLIENTDIR}/SymuxClient.0
