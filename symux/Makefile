# $Id: Makefile,v 1.5 2002/03/22 16:40:22 dijkstra Exp $

PROG=	monmux
SRCS=	monmux.c readconf.c muxnet.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -L../lib -L/usr/local/lib -lrrd -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-g -I../lib -I/usr/local/include -Wall
DEBUG=-g
.include <bsd.prog.mk>
