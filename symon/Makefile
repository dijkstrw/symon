# $Id: Makefile,v 1.11 2001/07/01 12:50:24 dijkstra Exp $

PROG=	mon
SRCS=	mon.c cpu.c mem.c ifstat.c disk.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -lkvm -L/usr/local/lib -lrrd
DPADD=	${LIBKVM}
CPPFLAGS+=-DMON_KVM -DNET_INET6 -Wall -I/usr/local/include -Wall

.include <bsd.prog.mk>
