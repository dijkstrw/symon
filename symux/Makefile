# $Id: Makefile,v 1.7 2002/04/01 20:16:04 dijkstra Exp $

PROG=	monmux
SRCS=	monmux.c readconf.c muxnet.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -L../lib -L/usr/local/lib -lrrd -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-W -g -I../lib -I/usr/local/include -Wall
DEBUG=-g
.include <bsd.prog.mk>
