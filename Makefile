# $Id: Makefile,v 1.5 2002/08/29 19:37:38 dijkstra Exp $

SUBDIR=	lib mon monmux 
V=2.1

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
	cd ..; \
	echo Exporting mon-${V}.tar.gz; \
	find $${workdir} -type f -print | egrep -v 'CVS|doc|clients|README|TODO|regress|#'| \
		tar -czvf mon-${V}.tar.gz -I -


_SUBDIRUSE: .USE
.if defined(SUBDIR)
	@for entry in ${SUBDIR}; do \
		echo "===> $${entry}"; \
		cd ${.CURDIR}/$${entry}; \
		${MAKE} ${.MAKEFLAGS} ${.TARGET}; \
	done
.endif

