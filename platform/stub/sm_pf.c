#include <stdlib.h>

#include "sylimits.h"
#include "data.h"
#include "error.h"

void
privinit_pf(void)
{
    fatal("pf module not available");
}
void
init_pf(struct stream *st)
{
    fatal("pf module not available");
}
void
gets_pf(void)
{
    fatal("pf module not available");
}
int
get_pf(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("pf module not available");

    /* NOT REACHED */
    return 0;
}
