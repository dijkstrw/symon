# $Id: Makefile.inc,v 1.14 2003/01/08 16:04:40 dijkstra Exp $

V=2.57

AR=	ar
CC=	cc
.ifdef DEBUG
CFLAGS+= -g -Wall
.else
CFLAGS+=-Wall
.endif
INSTALL=install
LORDER=	lorder
NROFF=	nroff
RANLIB= ranlib
STRIP=  strip
POD2MAN= pod2man

BINDIR= libexec
MANDIR=	man
SHRDIR= share/symon
WEBDIR= ${SHRDIR}/web
CLIENTDIR= ${SHRDIR}/client

.ifndef PREFIX
PREFIX=/usr/local
.endif

.ifndef SYSCONDIR
SYSCONFDIR=/etc
.endif

.SUFFIXES: .c .o .8 .cat8 .pm .cat3p

.c.o:
	${CC} ${CFLAGS} -c $<

.8.cat8:
	${NROFF} -mandoc $< > $@ || (rm -f $@; false)
		
.pm.cat3p:
	${POD2MAN} $< > $@.tmp || (rm -f $@; false)
	${NROFF} -mandoc $@.tmp > $@ || (rm -f $@; false)
	rm -f $@.tmp


