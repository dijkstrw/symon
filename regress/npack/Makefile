# $Id: Makefile,v 1.1 2003/10/05 07:52:37 dijkstra Exp $

TARGET=		npack
SRCS= 		npack.c
DEPS=           ../../lib/libsymon.a
CFLAGS= 	-I../../lib
LIBS=		-L../../lib -lsymon

../../lib/libsymon.a: 
	cd ../../lib; make libsymon.a

.include "../Makefile.inc"