/*
 * The percentages function was written by William LeFebvre and is part
 * of the 'top' utility. His copyright statement is below.
 */

/*
 *  Top users/processes display for Unix
 *  Version 3
 *
 *  This program may be freely redistributed,
 *  but this entire comment MUST remain intact.
 *
 *  Copyright (c) 1984, 1989, William LeFebvre, Rice University
 *  Copyright (c) 1989, 1990, 1992, William LeFebvre, Northwestern University
 */

#include <sys/types.h>

#define UQUAD_MAX ((u_int64_t)0-1)
#define QUAD_MAX ((int64_t)(UQUAD_MAX >> 1))

/*
 *  percentages(cnt, out, new, old, diffs) - calculate percentage change
 *      between array "old" and "new", putting the percentages i "out".
 *      "cnt" is size of each array and "diffs" is used for scratch space.
 *      The array "old" is updated on each call.
 *      The routine assumes modulo arithmetic.  This function is especially
 *      useful on BSD mchines for calculating cpu state percentages.
 */
int
percentages(int cnt, int64_t *out, int64_t *new, int64_t *old, int64_t *diffs)
{
    int64_t change, total_change, *dp, half_total;
    int i;

    /* initialization */
    total_change = 0;
    dp = diffs;

    /* calculate changes for each state and the overall change */
    for (i = 0; i < cnt; i++) {
        if ((change = *new - *old) < 0) {
            /* this only happens when the counter wraps */
            change = (QUAD_MAX - *old) + *new;
        }
        total_change += (*dp++ = change);
        *old++ = *new++;
    }

    /* avoid divide by zero potential */
    if (total_change == 0)
        total_change = 1;

    /* calculate percentages based on overall change, rounding up */
    half_total = total_change / 2l;
    for (i = 0; i < cnt; i++)
        *out++ = ((*diffs++ * 1000 + half_total) / total_change);

    /* return the total in case the caller wants to use it */
    return (total_change);
}
