# $Id: Makefile,v 1.9 2002/09/14 15:58:45 dijkstra Exp $

SUBDIR=	lib symon symux symon2web

.if make(clean)
SUBDIR+= ports/symon
.endif

.include "Makefile.inc"

all: _SUBDIRUSE
clean: _SUBDIRUSE
install: _SUBDIRUSE

# Not all the stuff that I'm working on is ready for release
dist: clean
	@workdir=`basename ${.CURDIR}`; \
	cd ports/symon; \
	rm -f distinfo; \
	${MAKE} clean; \
	cd ../../..; \
	echo Exporting symon-${V}.tar.gz; \
	find $${workdir} -type f -print | egrep -v 'CVS|doc|clients|README|regress|#'| \
		tar -czvf /tmp/symon-${V}.tar.gz -I -; \
	cp /tmp/symon-${V}.tar.gz /usr/ports/distfiles/; \
	cd $${workdir}/ports/symon; \
	${MAKE} makesum; \
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

