# $Id: Makefile,v 1.3 2003/12/20 16:30:44 dijkstra Exp $

TARGET=		npack
SRCS=		npack.c
DEPS=           ../../lib/libsymon.a
CFLAGS=	-I../../lib
LIBS=		-L../../lib -lsymon

.include "../Makefile.inc"

../../lib/libsymon.a:
	cd ../../lib; make libsymon.a
