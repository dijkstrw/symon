/*
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
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "conf.h"
#include "error.h"
#include "percentages.h"
#include "symon.h"
#include "xmalloc.h"

/* Globals for this module all start with cpw_ */
static void *cpw_buf = NULL;
static int cpw_size = 0;
static int cpw_maxsize = 0;

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
    if (CPUSTATES > sscanf(line, "%" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 "\n",
                           &st->parg.cp.time[CP_USER],
                           &st->parg.cp.time[CP_NICE],
                           &st->parg.cp.time[CP_SYS],
                           &st->parg.cp.time[CP_IDLE],
                           &st->parg.cp.time[CP_IOWAIT],
                           &st->parg.cp.time[CP_HARDIRQ],
                           &st->parg.cp.time[CP_SOFTIRQ],
                           &st->parg.cp.time[CP_STEAL])) {
      /* /proc/stat might not support steal */
      st->parg.cp.time[CP_STEAL] = 0;
      if ((CPUSTATES - 1) > sscanf(line, "%" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 " %" SCNd64 "\n",
				   &st->parg.cp.time[CP_USER],
				   &st->parg.cp.time[CP_NICE],
				   &st->parg.cp.time[CP_SYS],
				   &st->parg.cp.time[CP_IDLE],
				   &st->parg.cp.time[CP_IOWAIT],
				   &st->parg.cp.time[CP_HARDIRQ],
				   &st->parg.cp.time[CP_SOFTIRQ])) {
        warning("could not parse cpu statistics for %.200s", &st->parg.cp.name);
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
