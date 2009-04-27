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
