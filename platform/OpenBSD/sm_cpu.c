/* $Id: sm_cpu.c,v 1.26 2007/07/09 12:54:18 dijkstra Exp $ */

/* The author of this code is Willem Dijkstra (wpd@xs4all.nl).
 *
 * The percentages function was written by William LeFebvre and is part
 * of the 'top' utility. His copyright statement is below.
 *
 * Copyright (c) 2001-2007 Willem Dijkstra
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
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

/*
 * Get current cpu statistics in percentages (total of all counts = 100.0)
 * and returns them in symon_buf as
 *
 * user : nice : system : interrupt : idle
 *
 * This module uses the sysctl interface and can run as any user.
 */

#include "conf.h"

#include <sys/dkstat.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "symon.h"
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

void
init_cpu(struct stream *st)
{
    char buf[SYMON_MAX_OBJSIZE];
#ifdef HAS_KERN_CPTIME2
    const char *errstr;
    int mib[2] = {CTL_HW, HW_NCPU};
    int ncpu;
    long num;
    size_t size = sizeof(ncpu);
#endif

    st->parg.cp.mib[0] = CTL_KERN;
    st->parg.cp.mib[1] = KERN_CPTIME;
    st->parg.cp.miblen = 2;

#ifdef HAS_KERN_CPTIME2
    if (sysctl(mib, 2, &ncpu, &size, NULL, 0) == -1) {
        warning("could not determine number of cpus: %.200s", strerror(errno));
        ncpu = 1;
    }

    num = strtonum(st->arg, 0, SYMON_MAXCPUID - 1, &errstr);
    if (errstr != NULL) {
        fatal("cpu(%.200s) is invalid: %.200s", st->arg, errstr);
    }

    if (ncpu > 1) {
        st->parg.cp.mib[0] = CTL_KERN;
        st->parg.cp.mib[1] = KERN_CPTIME2;
        st->parg.cp.mib[2] = num;
        st->parg.cp.miblen = 3;
        if (st->parg.cp.mib[2] >= ncpu) {
            fatal("cpu(%d) is not present", st->parg.cp.mib[2]);
        }
    }
#endif

    /* Call get_cpu once to fill the cp_old structure */
    get_cpu(buf, sizeof(buf), st);

    info("started module cpu(%.200s)", st->arg);
}

void
gets_cpu()
{
    /* EMPTY */
}

int
get_cpu(char *symon_buf, int maxlen, struct stream *st)
{
    int i, total;
    size_t len;

    len = sizeof(st->parg.cp.time2);
    if (sysctl(st->parg.cp.mib, st->parg.cp.miblen, &st->parg.cp.time2, &len, NULL, 0) < 0) {
        warning("%s:%d: sysctl kern.cp_time* for cpu%d failed", __FILE__, __LINE__,
          st->parg.cp.mib[2]);
        return 0;
    }

    /* sysctl will return 32 bit longs for CPTIME and 64 bit longs for CPTIME2 */
    if (st->parg.cp.mib[1] == KERN_CPTIME) {
        bcopy(st->parg.cp.time2, st->parg.cp.time1, sizeof(st->parg.cp.time1));
        for (i = 0; i < CPUSTATES; i++) {
            st->parg.cp.time2[i] = (int64_t) st->parg.cp.time1[i];
        }
    }

    /* convert cp_time counts to percentages */
    total = percentages(CPUSTATES, st->parg.cp.states, st->parg.cp.time2, st->parg.cp.old, st->parg.cp.diff);

    return snpack(symon_buf, maxlen, st->arg, MT_CPU,
                  (double) (st->parg.cp.states[CP_USER] / 10.0),
                  (double) (st->parg.cp.states[CP_NICE] / 10.0),
                  (double) (st->parg.cp.states[CP_SYS] / 10.0),
                  (double) (st->parg.cp.states[CP_INTR] / 10.0),
                  (double) (st->parg.cp.states[CP_IDLE] / 10.0));
}
