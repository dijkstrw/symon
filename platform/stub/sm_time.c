#include "error.h"
#include "symon.h"

void
init_time(struct stream *st)
{
    fatal("time module not available");
}

int
get_time(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("time module not available");

    /* NOT REACHED */
    return 0;
}
