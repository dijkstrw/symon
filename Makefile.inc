# $Id: Makefile.inc,v 1.3 2002/08/31 14:59:57 dijkstra Exp $

AR=	ar
CC=	cc
.ifdef DEBUG
CFLAGS= -g -Wall
.else
CFLAGS= -O2 -Wall
.endif
.ifdef KVM
CFLAGS+=-DMON_KVM
.endif
INSTALL=install
LORDER=	lorder
NROFF=	nroff
RANLIB= ranlib
STRIP=  strip

BINDIR= libexec
MANDIR=	man
SHRDIR= share/mon

.ifndef PREFIX
PREFIX=/usr/local
.endif

.SUFFIXES: .c .o .8 .cat8

.c.o:
	${CC} ${CFLAGS} -c $<

.8.cat8:
	${NROFF} -mandoc $< > $@ || (rm -f $@; false)
