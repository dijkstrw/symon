# $Id: Makefile,v 1.8 2002/05/09 12:39:53 dijkstra Exp $

PROG=	monmux
SRCS=	monmux.c readconf.c muxnet.c
MAN= 	monmux.8
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -L../lib -L/usr/local/lib -lrrd -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-W -g -I../lib -I/usr/local/include -Wall
DEBUG=-g
.include <bsd.prog.mk>
