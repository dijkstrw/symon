# $Id: Makefile,v 1.15 2002/03/29 16:29:38 dijkstra Exp $

PROG=	mon
SRCS=	mon.c cpu.c mem.c if.c io.c readconf.c monnet.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -g -lkvm -L../lib -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-g -DMON_KVM -DNET_INET6 -Wall -I ../lib -Wall

.include <bsd.prog.mk>
