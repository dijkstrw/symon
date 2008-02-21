# $Id: Makefile.inc,v 1.5 2008/02/21 09:37:47 dijkstra Exp $

# LORDER and TSORT are non-functional and not needed on Linux. Tinker with
# these at your own peril.
LORDER=echo
TSORT=cat

BINDIR?=bin

INSTALLUSER?=root
INSTALLGROUPFILE?=bin
INSTALLGROUPDIR?=bin
