/* $Id: sm_sensor.c,v 1.2 2005/10/18 19:58:12 dijkstra Exp $ */

#include <stdlib.h>

#include "sylimits.h"
#include "data.h"
#include "error.h"

void
privinit_sensor()
{
    fatal("sensor module not available");
}
void
init_sensor(struct stream *st)
{
    fatal("sensor module not available");
}

int
get_sensor(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("sensor module not available");

    /* NOT REACHED */
    return 0;
}
