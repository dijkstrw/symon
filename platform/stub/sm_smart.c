#include <stdlib.h>

#include "sylimits.h"
#include "data.h"
#include "error.h"

void
init_smart(struct stream *st)
{
    fatal("smart module not available");
}

void
privinit_smart(struct stream *st)
{
    fatal("smart module not available");
}

void
gets_smart(void)
{
    fatal("smart module not available");
}

int
get_smart(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("io module not available");

    /* NOT REACHED */
    return 0;
}
