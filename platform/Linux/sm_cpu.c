/* $Id: sm_cpu.c,v 1.1 2004/08/08 17:21:18 dijkstra Exp $ */

/* The author of this code is Willem Dijkstra (wpd@xs4all.nl).
 *
 * The percentages function was written by William LeFebvre and is part
 * of the 'top' utility. His copyright statement is below.
 *
 * Copyright (c) 2001-2004 Willem Dijkstra
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

#define CPUSTATES 4
#define CP_USER 0
#define CP_NICE 1
#define CP_SYS  2
#define CP_IDLE 3

__BEGIN_DECLS
int percentages(int, int *, long *, long *, long *);
__END_DECLS

/* Globals for this module all start with cp_ */
static void *cp_buf = NULL;
static int cp_size = 0;
static int cp_maxsize = 0;
static long cp_time[SYMON_MAXCPUS][CPUSTATES];
static long cp_old[SYMON_MAXCPUS][CPUSTATES];
static long cp_diff[SYMON_MAXCPUS][CPUSTATES];
static int cp_states[SYMON_MAXCPUS][CPUSTATES];
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
	if ((change = *new - *old) < 0) {
	    /* this only happens when the counter wraps */
	    change = ((unsigned int) *new - (unsigned int) *old);
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
    return total_change;
}
/* Prepare cpu module for use */
void
init_cpu(char *s)
{
    char buf[SYMON_MAX_OBJSIZE];

    if (cp_buf == NULL) {
	cp_maxsize = SYMON_MAX_OBJSIZE;
	cp_buf = xmalloc(cp_maxsize);
    }

    gets_cpu();
    /* Call get_cpu once to fill the cp_old structure */
    get_cpu(buf, sizeof(buf), s);

    info("started module cpu(%.200s)", s);
}
void
gets_cpu()
{
    int fd;

    if ((fd = open("/proc/stat", O_RDONLY)) < 0) {
	warning("cannot access /proc/stat: %.200s", strerror(errno));
	return;
    }

    bzero(cp_buf, cp_maxsize);
    cp_size = read(fd, cp_buf, cp_maxsize);
    close(fd);

    if (cp_size == cp_maxsize) {
	/* buffer is too small to hold all interface data */
	cp_maxsize += SYMON_MAX_OBJSIZE;
	if (cp_maxsize > SYMON_MAX_OBJSIZE * SYMON_MAX_DOBJECTS) {
	    fatal("%s:%d: dynamic object limit (%d) exceeded for cp data",
		  __FILE__, __LINE__, SYMON_MAX_OBJSIZE * SYMON_MAX_DOBJECTS);
	}
	cp_buf = xrealloc(cp_buf, cp_maxsize);
	gets_cpu();
	return;
    }

    if (cp_size == -1) {
	warning("could not read if statistics from /proc/stat: %.200s", strerror(errno));
    }
}
/* Get new cpu measurements */
int
get_cpu(char *symon_buf, int maxlen, char *s)
{
    char cpuname[SYMON_MAX_OBJSIZE];
    int nr = 0;
    char *line;

    if (cp_size <= 0) {
	return 0;
    }

    bzero(&cpuname, sizeof(cpuname));
    if (s != NULL && isdigit(*s)) {
	snprintf(cpuname, sizeof(cpuname), "cpu%s", s);
	nr = strtol(s, NULL, 10);
    } else {
	snprintf(cpuname, sizeof(cpuname), "cpu");
	nr = 0;
    }

    if ((line = strstr(cp_buf, cpuname)) == NULL) {
	warning("could not find %s", &cpuname);
	return 0;
    }

    line += strlen(cpuname);
    if (4 > sscanf(line, "%lu %lu %lu %lu\n",
		   &cp_time[nr][CP_USER], &cp_time[nr][CP_NICE],
		   &cp_time[nr][CP_SYS], &cp_time[nr][CP_IDLE])) {
	warning("could not parse cpu statistics for %.200s", &cpuname);
	return 0;
    }

    percentages(CPUSTATES, cp_states[nr], cp_time[nr], cp_old[nr], cp_diff[nr]);

    return snpack(symon_buf, maxlen, s, MT_CPU,
		  (double) (cp_states[nr][CP_USER] / 10.0),
		  (double) (cp_states[nr][CP_NICE] / 10.0),
		  (double) (cp_states[nr][CP_SYS] / 10.0),
		  (double) (0),
		  (double) (cp_states[nr][CP_IDLE] / 10.0));
}
