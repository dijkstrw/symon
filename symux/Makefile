# $Id: Makefile,v 1.9 2002/05/31 14:24:37 dijkstra Exp $

PROG=	monmux
SRCS=	monmux.c readconf.c muxnet.c share.c
MAN= 	monmux.8
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -L../lib -L/usr/local/lib -lrrd -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-W -g -I../lib -I/usr/local/include -Wall
DEBUG=-g
.include <bsd.prog.mk>
