# $Id: Makefile,v 1.2 2003/10/10 15:19:58 dijkstra Exp $

TARGET=		npack
SRCS= 		npack.c
DEPS=           ../../lib/libsymon.a
CFLAGS= 	-I../../lib
LIBS=		-L../../lib -lsymon

.include "../Makefile.inc"

../../lib/libsymon.a: 
	cd ../../lib; make libsymon.a

