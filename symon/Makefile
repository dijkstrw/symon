#	$OpenBSD: Makefile,v 1.9 2000/06/14 01:05:04 itojun Exp $

PROG=	mon
SRCS=	mon.c
BINGRP=	kmem
BINMODE=2555
LDADD= -lkvm
DPADD=	${LIBKVM}

INET6?= yes
.if (${INET6} != "no")
CPPFLAGS+=      -DINET6
.endif

.include <bsd.prog.mk>
