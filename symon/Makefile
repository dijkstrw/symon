#	$OpenBSD: Makefile,v 1.9 2000/06/14 01:05:04 itojun Exp $

PROG=	mon
SRCS=	mon.c cpu.c mem.c ifstat.c
NOMAN=	No man page yet.
BINGRP=	kmem
BINMODE=2555
LDADD= -lkvm -L/usr/local/lib -lrrd 
DPADD=	${LIBKVM}
CPPFLAGS+=-DMON_KVM -DMEM_SWAP -DNET_INET6 -Wall -I/usr/local/include  

.include <bsd.prog.mk>
