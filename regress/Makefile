# $Id: Makefile,v 1.4 2003/10/10 15:19:56 dijkstra Exp $

SUBDIR=	npack share

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

