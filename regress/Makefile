# $Id: Makefile,v 1.5 2003/12/20 16:30:44 dijkstra Exp $

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
