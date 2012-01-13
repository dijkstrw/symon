/*
 * Copyright (c) 2007-2012 Willem Dijkstra
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
 * Get current disk transfer statistics from kernel and return them in
 * symon_buf as
 *
 * total nr of transfers : total seeks : total bytes transferred
 */

#include "conf.h"

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/disk.h>

#ifdef HAS_HW_IOSTATS
#include <sys/iostat.h>
#endif

#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "error.h"
#include "symon.h"
#include "xmalloc.h"

/* Globals for this module start with io_ */
static int io_dks = 0;
static int io_maxdks = 0;

#ifndef HAS_HW_IOSTATS
static struct disk_sysctl *io_dkstats = NULL;
void
gets_io()
{
    int mib[3];
    size_t size;

    /* read size */
    mib[0] = CTL_HW;
    mib[1] = HW_DISKSTATS;
    mib[2] = sizeof (struct disk_sysctl);
    size = 0;
    if (sysctl(mib, 3, NULL, &size, NULL, 0) < 0) {
        fatal("%s:%d: io can't get hw.diskstats"
              __FILE__, __LINE__);
    }
    io_dks = size / sizeof (struct disk_sysctl);

    /* adjust buffer if necessary */
    if (io_dks > io_maxdks) {
        io_maxdks = io_dks;

        if (io_maxdks > SYMON_MAX_DOBJECTS) {
            fatal("%s:%d: dynamic object limit (%d) exceeded for diskstat structures",
                  __FILE__, __LINE__, SYMON_MAX_DOBJECTS);
        }

        io_dkstats = xrealloc(io_dkstats, io_maxdks * sizeof(struct disk_sysctl));
    }

    /* read structure  */
    size = io_maxdks * sizeof(struct disk_sysctl);
    if (sysctl(mib, 3, io_dkstats, &size, NULL, 0) < 0) {
        fatal("%s:%d: io can't get hw.diskstats"
              __FILE__, __LINE__);
    }
}
#else
static struct io_sysctl *io_dkstats = NULL;
void
gets_io()
{
    int mib[3];
    size_t size;

    /* read size */
    mib[0] = CTL_HW;
    mib[1] = HW_IOSTATS;
    mib[2] = sizeof(struct io_sysctl);
    size = 0;
    if (sysctl(mib, 3, NULL, &size, NULL, 0) < 0) {
        fatal("%s:%d: io can't get hw.iostats"
              __FILE__, __LINE__);
    }

    io_dks = size / sizeof (struct io_sysctl);

    /* adjust buffer if necessary */
    if (io_dks > io_maxdks) {
        io_maxdks = io_dks;

        if (io_maxdks > SYMON_MAX_DOBJECTS) {
            fatal("%s:%d: dynamic object limit (%d) exceeded for diskstat structures",
                  __FILE__, __LINE__, SYMON_MAX_DOBJECTS);
        }

        io_dkstats = xrealloc(io_dkstats, io_maxdks * sizeof(struct io_sysctl));
    }

    /* read structure  */
    size = io_maxdks * sizeof(struct io_sysctl);
    if (sysctl(mib, 3, io_dkstats, &size, NULL, 0) < 0) {
        fatal("%s:%d: io can't get hw.iostats"
              __FILE__, __LINE__);
    }
}
#endif
void
init_io(struct stream *st)
{
    info("started module io(%.200s)", st->arg);
}

int
get_io(char *symon_buf, int maxlen, struct stream *st)
{
    int i;

#ifndef HAS_HW_IOSTATS
    for (i = 0; i < io_maxdks; i++)
        if (strncmp(io_dkstats[i].dk_name, st->arg,
                    sizeof(io_dkstats[i].dk_name)) == 0)
            return snpack(symon_buf, maxlen, st->arg, MT_IO2,
                          io_dkstats[i].dk_rxfer,
                          io_dkstats[i].dk_wxfer,
                          io_dkstats[i].dk_seek,
                          io_dkstats[i].dk_rbytes,
                          io_dkstats[i].dk_wbytes);
#else
    for (i = 0; i < io_maxdks; i++)
        if (strncmp(io_dkstats[i].name, st->arg,
                    sizeof(io_dkstats[i].name)) == 0)
            return snpack(symon_buf, maxlen, st->arg, MT_IO2,
                          io_dkstats[i].rxfer,
                          io_dkstats[i].wxfer,
                          io_dkstats[i].seek,
                          io_dkstats[i].rbytes,
                          io_dkstats[i].wbytes);
#endif

    return 0;
}
