# $Id: Makefile,v 1.12 2003/10/10 15:19:45 dijkstra Exp $

SUBDIR=	lib symon symux client

.if make(clean)
SUBDIR+= ports/symon
.endif

.include "Makefile.inc"

all: _SUBDIRUSE
clean: _SUBDIRUSE
install: _SUBDIRUSE

dist: clean
	@workdir=`basename ${.CURDIR}`; \
	cd ports/symon; \
	rm -f distinfo; \
	${MAKE} clean; \
	cd ../../..; \
	echo Exporting symon-${V}.tar.gz; \
	find $${workdir} -type f -print | egrep -v 'CVS|README|regress|#'| \
		tar -czvf /tmp/symon-${V}.tar.gz -I -; \
	cp /tmp/symon-${V}.tar.gz /usr/ports/distfiles/; \
	cd $${workdir}/ports/symon; \
	${MAKE} makesum; \
	chown dijkstra.dijkstra distinfo; \
	cd ..; \
	find symon -type f -print | egrep -v 'CVS' | \
		tar -czvf /tmp/ports-symon-${V}.tar.gz -I -; \
	cd ../..

_SUBDIRUSE: .USE
.if defined(SUBDIR)
	@for entry in ${SUBDIR}; do \
		echo "===> $${entry}"; \
		cd ${.CURDIR}/$${entry}; \
		${MAKE} ${.MAKEFLAGS} ${.TARGET}; \
	done
.endif

