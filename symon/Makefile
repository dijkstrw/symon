# $Id: Makefile,v 1.16 2002/03/31 14:27:47 dijkstra Exp $

PROG=	mon
SRCS=	mon.c cpu.c mem.c if.c io.c readconf.c monnet.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -lkvm -L../lib -lmon
DPADD=	${LIBKVM}
CPPFLAGS+=-Wall -W -Wtraditional -Wpointer-arith -Wbad-function-cast -DMON_KVM -DNET_INET6 -Wall -I ../lib -Wall

.include <bsd.prog.mk>
