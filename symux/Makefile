# $Id: Makefile,v 1.6 2002/03/31 14:27:50 dijkstra Exp $

PROG=	monmux
SRCS=	monmux.c readconf.c muxnet.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -L../lib -L/usr/local/lib -lrrd -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-pg -g -I../lib -I/usr/local/include -Wall
DEBUG=-g
.include <bsd.prog.mk>
