V=2.90

AR?=	ar
CC?=	cc
CFLAGS+=-g -Wall -Wpedantic -Wextra -Wno-unused-parameter -Werror
INSTALL?=install
LORDER?=lorder
TSORT?=tsort
NROFF?=nroff
RANLIB?=ranlib
POD2MAN?=pod2man

RRDDIR?=/usr/local
BINDIR?=libexec
MANDIR?=man
SHRDIR?=share/symon
EXADIR?=share/examples/symon
CLIENTDIR?=${SHRDIR}/client

PREFIX?=/usr/local
SYSCONFDIR?=/etc

INSTALLUSER?=root
INSTALLGROUPDIR?=bin
INSTALLGROUPFILE?=wheel

.SUFFIXES: .c .o .8 .cat8 .pm .cat3p

.c.o:
	${CC} ${CFLAGS} -o $*.o -c $<

.8.cat8:
	${NROFF} -mandoc $< > $@ || (rm -f $@; false)

.pm.cat3p:
	${POD2MAN} $< > $@.tmp || (rm -f $@; false)
	${NROFF} -mandoc $@.tmp > $@ || (rm -f $@; false)
	rm -f $@.tmp
