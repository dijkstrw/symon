# $Id: Makefile.inc,v 1.10 2002/10/18 12:29:09 dijkstra Exp $

V=2.52

AR=	ar
CC=	cc
.ifdef DEBUG
CFLAGS+= -g -Wall
.else
CFLAGS+= -O2 -Wall
.endif
INSTALL=install
LORDER=	lorder
NROFF=	nroff
RANLIB= ranlib
STRIP=  strip

BINDIR= libexec
MANDIR=	man
SHRDIR= share/symon
WEBDIR= ${SHRDIR}/web

.ifndef PREFIX
PREFIX=/usr/local
.endif

.ifndef SYSCONDIR
SYSCONFDIR=/etc
.endif

.SUFFIXES: .c .o .8 .cat8

.c.o:
	${CC} ${CFLAGS} -c $<

.8.cat8:
	${NROFF} -mandoc $< > $@ || (rm -f $@; false)
