# $Id: Makefile.inc,v 1.27 2004/09/25 10:21:56 dijkstra Exp $

V=2.67
OS!=uname -s

AR=	ar
CC=	cc
.ifdef DEBUG
CFLAGS+=-g -Wall
.else
CFLAGS+=-Wall
.endif
INSTALL=install
LORDER=	lorder
TSORT=  tsort
NROFF=	nroff
RANLIB= ranlib
STRIP=  strip
POD2MAN= pod2man

RRDDIR= /usr/local
BINDIR= libexec
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
