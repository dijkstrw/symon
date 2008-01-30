/* $Id: sm_cpuiow.c,v 1.2 2008/01/30 14:29:31 dijkstra Exp $ */

/* The author of this code is Willem Dijkstra (wpd@xs4all.nl).
 *
 * The percentages function was written by William LeFebvre and is part
 * of the 'top' utility. His copyright statement is below.
 *
 * Copyright (c) 2001-2008 Willem Dijkstra
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
 * user : nice : system : interrupt : idle : iowait
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "conf.h"
#include "error.h"
#include "symon.h"
#include "xmalloc.h"

__BEGIN_DECLS
static int percentages(int, int64_t *, int64_t *, int64_t *, int64_t *);
__END_DECLS

/* Globals for this module all start with cpw_ */
static void *cpw_buf = NULL;
static int cpw_size = 0;
static int cpw_maxsize = 0;
/*
 *  percentages(cnt, out, new, old, diffs) - calculate percentage change
 *      between array "old" and "new", putting the percentages i "out".
 *      "cnt" is size of each array and "diffs" is used for scratch space.
 *      The array "old" is updated on each call.
 *      The routine assumes modulo arithmetic.  This function is especially
 *      useful on BSD mchines for calculating cpu state percentages.
 */
static int
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
init_cpuiow(struct stream *st)
{
    char buf[SYMON_MAX_OBJSIZE];

    if (cpw_buf == NULL) {
        cpw_maxsize = SYMON_MAX_OBJSIZE;
        cpw_buf = xmalloc(cpw_maxsize);
    }

    if (st->arg != NULL && isdigit(*st->arg)) {
        snprintf(st->parg.cpw.name, sizeof(st->parg.cpw.name), "cpu%s", st->arg);
    } else {
        snprintf(st->parg.cpw.name, sizeof(st->parg.cpw.name), "cpu");
    }

    gets_cpuiow();
    get_cpuiow(buf, sizeof(buf), st);

    info("started module cpuiow(%.200s)", st->arg);
}

void
gets_cpuiow()
{
    int fd;

    if ((fd = open("/proc/stat", O_RDONLY)) < 0) {
        warning("cannot access /proc/stat: %.200s", strerror(errno));
        return;
    }

    bzero(cpw_buf, cpw_maxsize);
    cpw_size = read(fd, cpw_buf, cpw_maxsize);
    close(fd);

    if (cpw_size == cpw_maxsize) {
        /* buffer is too small to hold all interface data */
        cpw_maxsize += SYMON_MAX_OBJSIZE;
        if (cpw_maxsize > SYMON_MAX_OBJSIZE * SYMON_MAX_DOBJECTS) {
            fatal("%s:%d: dynamic object limit (%d) exceeded for cp data",
                  __FILE__, __LINE__, SYMON_MAX_OBJSIZE * SYMON_MAX_DOBJECTS);
        }
        cpw_buf = xrealloc(cpw_buf, cpw_maxsize);
        gets_cpuiow();
        return;
    }

    if (cpw_size == -1) {
        warning("could not read if statistics from /proc/stat: %.200s", strerror(errno));
    }
}

int
get_cpuiow(char *symon_buf, int maxlen, struct stream *st)
{
    char *line;

    if (cpw_size <= 0) {
        return 0;
    }

    if ((line = strstr(cpw_buf, st->parg.cpw.name)) == NULL) {
        warning("could not find %s", st->parg.cpw.name);
        return 0;
    }

    line += strlen(st->parg.cpw.name);
    if (CPUSTATES > sscanf(line, "%llu %llu %llu %llu %llu %llu %llu %llu\n",
                           &st->parg.cpw.time[CP_USER],
                           &st->parg.cpw.time[CP_NICE],
                           &st->parg.cpw.time[CP_SYS],
                           &st->parg.cpw.time[CP_IDLE],
                           &st->parg.cpw.time[CP_IOWAIT],
                           &st->parg.cpw.time[CP_HARDIRQ],
                           &st->parg.cpw.time[CP_SOFTIRQ],
                           &st->parg.cpw.time[CP_STEAL])) {
        /* /proc/stat might not support steal */
        st->parg.cpw.time[CP_STEAL] = 0;
        if ((CPUSTATES - 1) > sscanf(line, "%llu %llu %llu %llu %llu %llu %llu\n",
                                     &st->parg.cpw.time[CP_USER],
                                     &st->parg.cpw.time[CP_NICE],
                                     &st->parg.cpw.time[CP_SYS],
                                     &st->parg.cpw.time[CP_IDLE],
                                     &st->parg.cpw.time[CP_IOWAIT],
                                     &st->parg.cpw.time[CP_HARDIRQ],
                                     &st->parg.cpw.time[CP_SOFTIRQ])) {
            warning("could not parse cpu statistics for %.200s", &st->parg.cpw.name);
            return 0;
        }
    }

    percentages(CPUSTATES, st->parg.cpw.states, st->parg.cpw.time,
                st->parg.cpw.old, st->parg.cpw.diff);

    return snpack(symon_buf, maxlen, st->arg, MT_CPUIOW,
                  (double) (st->parg.cpw.states[CP_USER] / 10.0),
                  (double) (st->parg.cpw.states[CP_NICE] / 10.0),
                  (double) (st->parg.cpw.states[CP_SYS] / 10.0),
                  (double) (st->parg.cpw.states[CP_HARDIRQ] +
                            st->parg.cpw.states[CP_SOFTIRQ] +
                            st->parg.cpw.states[CP_STEAL]) / 10.0,
                  (double) (st->parg.cpw.states[CP_IDLE] / 10.0),
                  (double) (st->parg.cpw.states[CP_IOWAIT] / 10.0));
}
