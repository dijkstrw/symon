# $Id: Makefile,v 1.3 2002/07/25 14:23:00 dijkstra Exp $

SUBDIR=	lib mon monmux 
V=2.0

.include "Makefile.inc"

all: _SUBDIRUSE
clean: _SUBDIRUSE
install: _SUBDIRUSE

# Not all the stuff that I'm working on is ready for release
dist: clean
	@workdir=`basename ${.CURDIR}`; \
	cd ..; \
	echo Exporting mon-${V}.tar.gz; \
	find $${workdir} -type f -print | egrep -v 'CVS|doc|clients|README|TODO|regress|mon2web|#'| \
		tar -czvf mon-${V}.tar.gz -I -


_SUBDIRUSE: .USE
.if defined(SUBDIR)
	@for entry in ${SUBDIR}; do \
		echo "===> $${entry}"; \
		cd ${.CURDIR}/$${entry}; \
		${MAKE} ${.MAKEFLAGS} ${.TARGET}; \
	done
.endif

