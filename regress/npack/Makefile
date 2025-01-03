.include "../../Makefile.inc"
.include "../../platform/${OS}/Makefile.inc"

LIBS= -L../../lib -lsym
SRCS= npack.c
OBJS+= ${SRCS:R:S/$/.o/g}
CFLAGS+= -I../../lib -I../../platform/${OS} -I.

all: npack

npack: ${OBJS}
	${CC} -o $@ ${OBJS} ${LIBS}
.ifndef DEBUG
	${STRIP} $@
.endif

clean:
	rm -f ${OBJS} npack npack.core

