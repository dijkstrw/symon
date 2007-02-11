/* $Id: sm_io.c,v 1.2 2007/02/11 20:07:32 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2006 Willem Dijkstra
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
 * Get current disk statistics from kernel and return them in symon_buf as
 *
 * total_rxfer, total_wxfer, total_seeks, total_rbytes, total_wbytes
 *
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "conf.h"
#include "xmalloc.h"
#include "error.h"
#include "symon.h"

/* Globals for this module start with io_ */
static void *io_buf = NULL;
static int io_size = 0;
static int io_maxsize = 0;
struct io_device_stats
{
    uint64_t read_issued;
    uint64_t read_merged;
    uint64_t read_sectors;
    uint64_t read_milliseconds;
    uint64_t write_issued;
    uint64_t write_merged;
    uint64_t write_sectors;
    uint64_t write_milliseconds;
    uint64_t progress_ios;
    uint64_t progress_milliseconds;
    uint64_t progress_weight;
};
#ifdef HAS_PROC_DISKSTATS
char *io_filename = "/proc/diskstats";
#else
#ifdef HAS_PROC_PARTITIONS
char *io_filename = "/proc/partitions";
#endif
#endif

#if defined(HAS_PROC_DISKSTATS) || defined(HAS_PROC_PARTITIONS)
void
init_io(struct stream *st)
{
    if (io_buf == NULL) {
        io_maxsize = SYMON_MAX_OBJSIZE;
        io_buf = xmalloc(io_maxsize);
    }

    info("started module io(%.200s)", st->arg);
}

void
gets_io()
{
    int fd;
    if ((fd = open(io_filename, O_RDONLY)) < 0) {
        warning("cannot access %.200s: %.200s", io_filename, strerror(errno));
        return;
    }

    bzero(io_buf, io_maxsize);
    io_size = read(fd, io_buf, io_maxsize);
    close(fd);

    if (io_size == io_maxsize) {
        /* buffer is too small to hold all interface data */
        io_maxsize += SYMON_MAX_OBJSIZE;
        if (io_maxsize > SYMON_MAX_OBJSIZE * SYMON_MAX_DOBJECTS) {
            fatal("%s:%d: dynamic object limit (%d) exceeded for io data",
                  __FILE__, __LINE__, SYMON_MAX_OBJSIZE * SYMON_MAX_DOBJECTS);
        }
        io_buf = xrealloc(io_buf, io_maxsize);
        gets_io();
        return;
    }

    if (io_size == -1) {
        warning("could not read io statistics from %.200s: %.200s", io_filename, strerror(errno));
    }
}

int
get_io(char *symon_buf, int maxlen, struct stream *st)
{
    char *line;
    struct io_device_stats stats;

    if (io_size <= 0) {
        return 0;
    }

    if ((line = strstr(io_buf, st->arg)) == NULL) {
        warning("could not find disk %s", st->arg);
        return 0;
    }

    line += strlen(st->arg);
    bzero(&stats, sizeof(struct io_device_stats));

    if (11 > sscanf(line, " %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
                    &stats.read_issued, &stats.read_merged,
                    &stats.read_sectors, &stats.read_milliseconds,
                    &stats.write_issued, &stats.write_merged,
                    &stats.write_sectors, &stats.write_milliseconds,
                    &stats.progress_ios, &stats.progress_milliseconds,
                    &stats.progress_weight)) {
#ifdef HAS_PROC_DISKSTATS
        if (4 > sscanf(line, " %llu %llu %llu %llu\n",
                       &stats.read_issued, &stats.read_sectors,
                       &stats.write_issued, &stats.write_sectors)) {
            warning("could not parse disk statistics for %.200s", st->arg);
            return 0;
        }
    }
#else
        warning("could not parse disk statistics for %.200s", st->arg);
        return 0;
    }
#endif

    return snpack(symon_buf, maxlen, st->arg, MT_IO2,
                  (stats.read_issued + stats.read_merged),
                  (stats.write_issued + stats.write_merged),
                  (uint64_t) 0,
                  (uint64_t)(stats.read_sectors * DEV_BSIZE),
                  (uint64_t)(stats.write_sectors * DEV_BSIZE));
}
#else
void
init_io(struct stream *st)
{
    fatal("io module not available");
}
void
gets_io()
{
    fatal("io module not available");
}
int
get_io(char *symon_buf, int maxlen, struct stream *st)
{
    fatal("io module not available");

    /* NOT REACHED */
    return 0;
}
#endif
