# $Id: Makefile,v 1.1 2001/08/20 14:40:12 dijkstra Exp $

PROG=	mon
SRCS=	monmux.c readconf.c xmalloc.c lex.c error.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -L/usr/local/lib -lrrd
DPADD=	${LIBKVM}
CPPFLAGS+=-g -Wall -I/usr/local/include -Wall
DEBUG=-g
.include <bsd.prog.mk>
