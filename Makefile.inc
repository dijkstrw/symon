# $Id: Makefile.inc,v 1.24 2004/08/07 12:21:36 dijkstra Exp $

V=2.67
OS!=uname -s

AR=	ar
CC=	cc
.ifdef DEBUG
CFLAGS+=-g -Wall -D__USE_XOPEN
.else
CFLAGS+=-Wall -D__USE_XOPEN
.endif
INSTALL=install
LORDER=	lorder
NROFF=	nroff
RANLIB= ranlib
STRIP=  strip
POD2MAN= pod2man

RRDDIR= /usr/local
BINDIR= sbin
MANDIR=	man
SHRDIR= share/symon
EXADIR= share/examples/symon
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
	${CC} ${CFLAGS} -o $*.o -c $<

.8.cat8:
	${NROFF} -mandoc $< > $@ || (rm -f $@; false)

.pm.cat3p:
	${POD2MAN} $< > $@.tmp || (rm -f $@; false)
	${NROFF} -mandoc $@.tmp > $@ || (rm -f $@; false)
	rm -f $@.tmp
