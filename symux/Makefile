# $Id: Makefile,v 1.3 2001/09/20 19:26:33 dijkstra Exp $

PROG=	monmux
SRCS=	monmux.c readconf.c xmalloc.c lex.c error.c net.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -L/usr/local/lib -lrrd
DPADD=	${LIBKVM}
CPPFLAGS+=-g -I/usr/local/include -Wall
DEBUG=-g
.include <bsd.prog.mk>
