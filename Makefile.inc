# $Id: Makefile.inc,v 1.2 2002/07/25 09:51:41 dijkstra Exp $

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

.ifndef PREFIX
PREFIX=/usr/local
.endif

.SUFFIXES: .c .o .8 .cat8

.c.o:
	${CC} ${CFLAGS} -c $<

.8.cat8:
	${NROFF} -mandoc $< > $@ || (rm -f $@; false)
