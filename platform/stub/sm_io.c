/* $Id: sm_io.c,v 1.2 2005/10/18 19:58:12 dijkstra Exp $ */

#include <stdlib.h>

#include "sylimits.h"
#include "data.h"
#include "error.h"

void
init_io(struct stream *st)
{
    fatal("io module not available");
}
void
gets_io()
{
    fatal("io module not available");
}
int
get_io(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("io module not available");

    /* NOT REACHED */
    return 0;
}
