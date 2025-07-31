MANDIR?=man
NROFF!= (if [ -x /usr/bin/mandoc ]; then echo mandoc; else echo nroff; fi)
