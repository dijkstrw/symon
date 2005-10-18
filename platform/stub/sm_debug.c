/* $Id: sm_debug.c,v 1.2 2005/10/18 19:58:12 dijkstra Exp $ */

#include <stdlib.h>

#include "sylimits.h"
#include "data.h"
#include "error.h"

void
init_debug(struct stream *st)
{
    fatal("debug module not available");
}
int
get_debug(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("debug module not available");

    /* NOT REACHED */
    return 0;
}
