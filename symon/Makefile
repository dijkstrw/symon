# $Id: Makefile,v 1.18 2002/04/09 05:37:36 dijkstra Exp $

PROG=	mon
SRCS=	mon.c cpu.c mem.c if.c io.c readconf.c monnet.c
MAN=    mon.8
BINGRP=	kmem
BINMODE=2555
LDADD=  -lkvm -L../lib -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-g -Wall -DMON_KVM -DNET_INET6 -Wall -I ../lib -Wall

.include <bsd.prog.mk>
