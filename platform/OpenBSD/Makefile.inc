SYMUX_LIBS?=-L/usr/X11R6/lib -lfontconfig
MANDIR?=man
NROFF!= (if [ -x /usr/bin/mandoc ]; then echo mandoc; else echo nroff; fi)