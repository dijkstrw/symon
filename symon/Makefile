# $Id: Makefile,v 1.13 2002/03/17 13:37:31 dijkstra Exp $

PROG=	mon
SRCS=	mon.c cpu.c mem.c if.c io.c readconf.c monnet.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -lkvm -L../lib -L/usr/local/lib -lrrd -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-DDEBUG -g -DMON_KVM -DNET_INET6 -Wall -I/usr/local/include -I ../lib -Wall
#CPPFLAGS+=-DDEBUG -g -Wall -I/usr/local/include -I ../lib -Wall

.include <bsd.prog.mk>
