/* $Id: sm_cpu.c,v 1.2 2004/08/08 17:21:18 dijkstra Exp $ */

#include <stdlib.h>

#include "error.h"

void
init_cpu(char *s)
{
    fatal("cpu module not available");
}

void
gets_cpu(char *s)
{
    fatal("cpu module not available");
}

int
get_cpu(char *symon_buf, int maxlen, char *s)
{
    fatal("cpu module not available");

    /* NOT REACHED */
    return 0;
}
