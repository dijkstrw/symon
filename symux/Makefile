# $Id: Makefile,v 1.4 2001/09/30 15:10:20 dijkstra Exp $

PROG=	monmux
SRCS=	monmux.c readconf.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -L../lib -L/usr/local/lib -lrrd -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-g -I../lib -I/usr/local/include -Wall
DEBUG=-g
.include <bsd.prog.mk>
