# $Id: Makefile.inc,v 1.7 2002/09/13 07:42:50 dijkstra Exp $

V=2.4

AR=	ar
CC=	cc
.ifdef DEBUG
CFLAGS= -g -Wall
.else
CFLAGS= -O2 -Wall
.endif
INSTALL=install
LORDER=	lorder
NROFF=	nroff
RANLIB= ranlib
STRIP=  strip

BINDIR= libexec
MANDIR=	man
SHRDIR= share/mon
WEBDIR= ${SHRDIR}/web

.ifndef PREFIX
PREFIX=/usr/local
.endif

.SUFFIXES: .c .o .8 .cat8

.c.o:
	${CC} ${CFLAGS} -c $<

.8.cat8:
	${NROFF} -mandoc $< > $@ || (rm -f $@; false)
