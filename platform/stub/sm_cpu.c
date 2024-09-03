#include <stdlib.h>

#include "sylimits.h"
#include "data.h"
#include "error.h"

void
init_cpu(struct stream *st)
{
    fatal("cpu module not available");
}
void
gets_cpu(void)
{
    fatal("cpu module not available");
}
int
get_cpu(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("cpu module not available");

    /* NOT REACHED */
    return 0;
}
