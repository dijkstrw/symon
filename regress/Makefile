# $Id: Makefile,v 1.3 2003/10/05 07:50:31 dijkstra Exp $

SUBDIR=	pack share

all: _SUBDIRUSE
clean: _SUBDIRUSE
regress: _SUBDIRUSE

_SUBDIRUSE: .USE
.if defined(SUBDIR)
	@for entry in ${SUBDIR}; do \
		echo "===> $${entry}"; \
		cd ${.CURDIR}/$${entry}; \
		${MAKE} ${.MAKEFLAGS} ${.TARGET}; \
	done
.endif

