# $Id: Makefile,v 1.2 2001/09/02 19:01:49 dijkstra Exp $

PROG=	mon
SRCS=	monmux.c readconf.c xmalloc.c lex.c error.c net.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -L/usr/local/lib -lrrd
DPADD=	${LIBKVM}
CPPFLAGS+=-g -I/usr/local/include -Wall
DEBUG=-g
.include <bsd.prog.mk>
