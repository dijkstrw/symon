# $Id: Makefile.inc,v 1.1 2002/07/20 14:28:28 dijkstra Exp $

AR=	ar
CC=	cc
CFLAGS= -O2 -Wall
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
