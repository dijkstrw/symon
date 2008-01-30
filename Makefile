# $Id: Makefile,v 1.15 2008/01/30 13:19:40 dijkstra Exp $

.ifdef SYMON_ONLY
SUBDIR= lib symon
.else
SUBDIR=	lib symon symux client
.endif

.include "Makefile.inc"

all: _SUBDIRUSE
clean: _SUBDIRUSE
install: _SUBDIRUSE

_SUBDIRUSE: .USE
.if defined(SUBDIR)
	@for entry in ${SUBDIR}; do \
		echo "===> $${entry}"; \
		cd ${.CURDIR}/$${entry}; \
		${MAKE} ${.MAKEFLAGS} ${.TARGET}; \
	done
.endif
