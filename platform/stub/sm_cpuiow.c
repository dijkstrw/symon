/* $Id: sm_cpuiow.c,v 1.1 2008/01/30 12:06:50 dijkstra Exp $ */

#include <stdlib.h>

#include "sylimits.h"
#include "data.h"
#include "error.h"

void
init_cpuiow(struct stream *st)
{
    fatal("cpuiow module not available");
}
void
gets_cpuiow()
{
    fatal("cpuiow module not available");
}
int
get_cpuiow(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("cpuiow module not available");

    /* NOT REACHED */
    return 0;
}
