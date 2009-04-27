#include <stdlib.h>

#include "sylimits.h"
#include "data.h"
#include "error.h"

void
init_proc(struct stream *st)
{
    fatal("proc module not available");
}

void
gets_proc()
{
    fatal("proc module not available");
}

void
privinit_proc()
{
    fatal("proc module not available");
}

int
get_proc(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("proc module not available");

    /* NOT REACHED */
    return 0;
}
