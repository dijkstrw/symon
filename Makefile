# $Id: Makefile,v 1.8 2002/09/10 18:32:44 dijkstra Exp $

SUBDIR=	lib mon monmux mon2web

.if make(clean)
SUBDIR+= ports/mon
.endif

.include "Makefile.inc"

all: _SUBDIRUSE
clean: _SUBDIRUSE
install: _SUBDIRUSE

# Not all the stuff that I'm working on is ready for release
dist: clean
	@workdir=`basename ${.CURDIR}`; \
	cd ports/mon; \
	rm -f distinfo; \
	${MAKE} clean; \
	cd ../../..; \
	echo Exporting mon-${V}.tar.gz; \
	find $${workdir} -type f -print | egrep -v 'CVS|doc|clients|README|regress|#'| \
		tar -czvf /tmp/mon-${V}.tar.gz -I -; \
	cp /tmp/mon-${V}.tar.gz /usr/ports/distfiles/; \
	cd $${workdir}/ports/mon; \
	${MAKE} makesum; \
	cd ..; \
	find mon -type f -print | egrep -v 'CVS' | \
		tar -czvf /tmp/ports-mon-${V}.tar.gz -I -; \
	cd ../..

_SUBDIRUSE: .USE
.if defined(SUBDIR)
	@for entry in ${SUBDIR}; do \
		echo "===> $${entry}"; \
		cd ${.CURDIR}/$${entry}; \
		${MAKE} ${.MAKEFLAGS} ${.TARGET}; \
	done
.endif

