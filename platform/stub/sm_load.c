#include <stdlib.h>

#include "sylimits.h"
#include "data.h"
#include "error.h"

void
init_load(struct stream *st)
{
    fatal("load module not available");
}
void
gets_load(void)
{
    fatal("load module not available");
}
int
get_load(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("load module not available");

    /* NOT REACHED */
    return 0;
}
