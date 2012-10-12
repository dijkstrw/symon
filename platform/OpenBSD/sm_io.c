/*
 * Copyright (c) 2001-2012 Willem Dijkstra
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

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/disk.h>

#include <limits.h>
#include <string.h>

#include "conf.h"
#include "error.h"
#include "symon.h"
#include "xmalloc.h"

/* Globals for this module start with io_ */
static char *io_dkstr = NULL;
static struct diskstats *io_dkstats = NULL;
static char **io_dknames = NULL;
static char **io_dkuids = NULL;
static int io_dks = 0;
static int io_maxdks = 0;
static size_t io_maxstr = 0;

void
gets_io()
{
    int mib[3];
    char *p;
    int dks;
    size_t size;
    size_t strsize;

    /* how much memory is needed */
    mib[0] = CTL_HW;
    mib[1] = HW_DISKCOUNT;
    size = sizeof(dks);
    if (sysctl(mib, 2, &dks, &size, NULL, 0) < 0) {
        fatal("%s:%d: sysctl failed: can't get hw.diskcount",
              __FILE__, __LINE__);
    }

    mib[0] = CTL_HW;
    mib[1] = HW_DISKNAMES;
    strsize = 0;
    if (sysctl(mib, 2, NULL, &strsize, NULL, 0) < 0) {
        fatal("%s:%d: sysctl failed: can't get hw.disknames",
              __FILE__, __LINE__);
    }

    /* increase buffers if necessary */
    if (dks > io_maxdks || strsize > io_maxstr) {
        io_maxdks = dks;
        io_maxstr = strsize;

        if (io_maxdks > SYMON_MAX_DOBJECTS) {
            fatal("%s:%d: dynamic object limit (%d) exceeded for diskstat structures",
                  __FILE__, __LINE__, SYMON_MAX_DOBJECTS);
        }

        if (io_maxstr > SYMON_MAX_OBJSIZE) {
            fatal("%s:%d: string size exceeded (%d)",
                  __FILE__, __LINE__, SYMON_MAX_OBJSIZE);
        }

        io_dkstats = xrealloc(io_dkstats, io_maxdks * sizeof(struct diskstats));
        io_dknames = xrealloc(io_dknames, io_maxdks * sizeof(char *));
        io_dkuids = xrealloc(io_dkuids, io_maxdks * sizeof(char *));
        io_dkstr = xrealloc(io_dkstr, io_maxstr + 1);
    }

    /* read data in anger */
    mib[0] = CTL_HW;
    mib[1] = HW_DISKNAMES;
    if (sysctl(mib, 2, io_dkstr, &io_maxstr, NULL, 0) < 0) {
        fatal("%s:%d: io can't get hw.disknames"
              __FILE__, __LINE__);
    }
    io_dkstr[io_maxstr] = '\0';

    mib[0] = CTL_HW;
    mib[1] = HW_DISKSTATS;
    size = io_maxdks * sizeof(struct diskstats);
    if (sysctl(mib, 2, io_dkstats, &size, NULL, 0) < 0) {
        fatal("%s:%d: io can't get hw.diskstats"
              __FILE__, __LINE__);
    }

    p = io_dkstr;
    io_dks = 0;

    io_dknames[io_dks] = p;

    while ((*p != '\0') && ((p - io_dkstr) < io_maxstr)) {
        if ((*p == ',') && (*p+1 != '\0')) {
            *p = '\0';
            io_dks++; p++;
            io_dknames[io_dks] = p;
            io_dkuids[io_dks] = NULL;
        }
        if ((*p == ':') && (*p+1 != '\0')) {
            *p = '\0';
            io_dkuids[io_dks] = p+1;
	}
        p++;
    }
}

void
init_io(struct stream *st)
{
    info("started module io(%.200s)", st->arg);
}

int
get_io(char *symon_buf, int maxlen, struct stream *st)
{
    int i;

    /* look for disk */
    for (i = 0; i <= io_dks; i++) {
        if ((strncmp(io_dknames[i], st->arg,
                    (io_dkstr + io_maxstr - io_dknames[i])) == 0)
        	    || (io_dkuids[i] && (strncmp(io_dkuids[i], st->arg,
                    (io_dkstr + io_maxstr - io_dkuids[i])) == 0)))
#ifdef HAS_IO2
            return snpack(symon_buf, maxlen, st->arg, MT_IO2,
                          io_dkstats[i].ds_rxfer,
                          io_dkstats[i].ds_wxfer,
                          io_dkstats[i].ds_seek,
                          io_dkstats[i].ds_rbytes,
                          io_dkstats[i].ds_wbytes);
#else
            return snpack(symon_buf, maxlen, st->arg, MT_IO1,
                          io_dkstats[i].ds_xfer,
                          io_dkstats[i].ds_seek,
                          io_dkstats[i].ds_bytes);
#endif
    }

    return 0;
}
