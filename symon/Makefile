# $Id: Makefile,v 1.17 2002/04/01 20:15:59 dijkstra Exp $

PROG=	mon
SRCS=	mon.c cpu.c mem.c if.c io.c readconf.c monnet.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -lkvm -L../lib -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-g -Wall -DMON_KVM -DNET_INET6 -Wall -I ../lib -Wall

.include <bsd.prog.mk>
