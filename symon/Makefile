# $Id: Makefile,v 1.10 2001/05/27 17:15:07 dijkstra Exp $

PROG=	mon
SRCS=	mon.c cpu.c mem.c ifstat.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD=  -lkvm -L/usr/local/lib -lrrd
DPADD=	${LIBKVM}
CPPFLAGS+=-DMON_KVM -DMEM_SWAP -DNET_INET6 -Wall -I/usr/local/include -Wall

.include <bsd.prog.mk>
