# $Id: Makefile,v 1.1 2002/05/31 14:25:00 dijkstra Exp $
.PATH:  ${.CURDIR}/../../monmux

PROG=	testshare 
SRCS=	testshare.c share.c
LDADD=  -g -L../../lib -lmon
CPPFLAGS+=-DDEBUG -g -Wall -I/usr/local/include -I ../../lib  -I ../../monmux
DEBUG=-g
.include <bsd.prog.mk>
