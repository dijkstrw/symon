/* $Id: sm_cpu.c,v 1.1 2004/08/07 12:21:36 dijkstra Exp $ */

/* The author of this code is Matthew Gream.
 *
 * The percentages function was written by William LeFebvre and is part
 * of the 'top' utility. His copyright statement is below.
 *
 * Copyright (c) 2004      Matthew Gream
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
 * This code is not re-entrant and UP only.
 *
 * This module uses the sysctl interface and can run as any user.
 */

#include <sys/param.h>
#include <sys/dkstat.h>
#include <sys/sysctl.h>

#include "error.h"
#include "symon.h"

__BEGIN_DECLS
int percentages(int, int *, long *, long *, long *);
__END_DECLS

/* Globals for this module all start with cp_ */
static char cp_time_mib_str[] = "kern.cp_time";
static int cp_time_mib[CTL_MAXNAME];
static size_t cp_time_len = 0;
static size_t cp_size;
static long cp_time[CPUSTATES];
static long cp_old[CPUSTATES];
static long cp_diff[CPUSTATES];
static int cp_states[CPUSTATES];
/*
 *  percentages(cnt, out, new, old, diffs) - calculate percentage change
 *      between array "old" and "new", putting the percentages i "out".
 *      "cnt" is size of each array and "diffs" is used for scratch space.
 *      The array "old" is updated on each call.
 *      The routine assumes modulo arithmetic.  This function is especially
 *      useful on BSD mchines for calculating cpu state percentages.
 */
int
percentages(int cnt, int *out, register long *new, register long *old, long *diffs)
{
    register int i;
    register long change;
    register long total_change;
    register long *dp;
    long half_total;

    /* initialization */
    total_change = 0;
    dp = diffs;

    /* calculate changes for each state and the overall change */
    for (i = 0; i < cnt; i++) {
        if (*new < *old)
            change = (ULONG_MAX - *old) + *new;
        else
            change = *new - *old;
	total_change += (*dp++ = change);
	*old++ = *new++;
    }

    /* avoid divide by zero potential */
    if (total_change == 0)
	total_change = 1;

    /* calculate percentages based on overall change, rounding up */
    half_total = total_change / 2;
    for (i = 0; i < cnt; i++)
	*out++ = ((*diffs++ * 1000 + half_total) / total_change);

    /* return the total in case the caller wants to use it */
    return total_change;
}
/* Prepare cpu module for use */
void
init_cpu(char *s)
{
    cp_time_len = CTL_MAXNAME;
    if (sysctlnametomib(cp_time_mib_str, cp_time_mib, &cp_time_len) < 0) {
	warning("sysctlnametomib for cpu failed");
        cp_time_len = 0;
    }

    cp_size = sizeof(cp_time);
    get_cpu(NULL, 0, NULL);

    info("started module cpu(%.200s)", s);
}
/* Get new cpu measurements */
int
get_cpu(char *symon_buf, int maxlen, char *s)
{
    if (!cp_time_len)
	return 0;

    if (sysctl(cp_time_mib, cp_time_len, &cp_time, &cp_size, NULL, 0) < 0) {
	warning("%s:%d: sysctl kern.cp_time failed", __FILE__, __LINE__);
	return 0;
    }

    /* convert cp_time counts to percentages */
    (void)percentages(CPUSTATES, cp_states, cp_time, cp_old, cp_diff);

    if (!symon_buf)
	return 0;

    return snpack(symon_buf, maxlen, s, MT_CPU,
		  (double) (cp_states[CP_USER] / 10.0),
		  (double) (cp_states[CP_NICE] / 10.0),
		  (double) (cp_states[CP_SYS] / 10.0),
		  (double) (cp_states[CP_INTR] / 10.0),
		  (double) (cp_states[CP_IDLE] / 10.0));
}