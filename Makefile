# $Id: Makefile,v 1.7 2002/09/09 22:00:26 dijkstra Exp $

SUBDIR=	lib mon monmux mon2web
V=2.4

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
	find $${workdir} -type f -print | egrep -v 'CVS|doc|clients|README|regress|#'| \
		tar -czvf mon-${V}.tar.gz -I -


_SUBDIRUSE: .USE
.if defined(SUBDIR)
	@for entry in ${SUBDIR}; do \
		echo "===> $${entry}"; \
		cd ${.CURDIR}/$${entry}; \
		${MAKE} ${.MAKEFLAGS} ${.TARGET}; \
	done
.endif

