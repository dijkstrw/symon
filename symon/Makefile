#	$OpenBSD: Makefile,v 1.9 2000/06/14 01:05:04 itojun Exp $

PROG=	mon
SRCS=	mon.c
# ?.PATH:	${.CURDIR}/../../sys/netiso
BINGRP=	kmem
BINMODE=2555
LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
