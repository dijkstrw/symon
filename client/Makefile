# $Id: Makefile,v 1.1 2002/11/08 15:40:25 dijkstra Exp $
.include "../Makefile.inc"

all: SymuxClient.cat3p

clean:
	rm -f SymuxClient.cat3p

install: SymuxClient.pm SymuxClient.cat3p getsymonitem.pl
	${INSTALL} -d -m 555 -g www -o www ${PREFIX}/${CLIENTDIR}
	${INSTALL} -c -m 555 -g bin   -o root getsymonitem.pl      ${PREFIX}/${CLIENTDIR}
	${INSTALL} -c -m 444 -g wheel -o root SymuxClient.pm       ${PREFIX}/${CLIENTDIR}
	${INSTALL} -c -m 444 -g wheel -o root SymuxClient.cat3p    ${PREFIX}/${CLIENTDIR}/SymuxClient.0

