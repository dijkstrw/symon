# $Id: Makefile,v 1.2 2003/12/20 16:30:44 dijkstra Exp $
.PATH:  ${.CURDIR}/../../monmux

PROG=	testshare
SRCS=	testshare.c share.c
LDADD=  -g -L../../lib -lmon
CPPFLAGS+=-DDEBUG -g -Wall -I/usr/local/include -I ../../lib  -I ../../monmux
DEBUG=-g
.include <bsd.prog.mk>
