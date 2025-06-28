/*
 * Copyright (c) 2003 Daniel Hartmeier
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

#include "conf.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/pool.h>

#include <errno.h>
#include <string.h>

#include "error.h"
#include "symon.h"

/* pool info */
static int mbpool_index = -1;
static int mclpools_index[MCLPOOLS];
static int mclpool_count = 0;
static struct kinfo_pool mbpool;

static int maxclusters;
static int iter;

void
init_mbuf(struct stream *st)
{
    int i, mib[4], npools;
    char pname[32];
    size_t size;

    /* go through all pools to identify mbuf and cluster pools */

    mib[0] = CTL_KERN;
    mib[1] = KERN_POOL;
    mib[2] = KERN_POOL_NPOOLS;
    size = sizeof(npools);

    if (sysctl(mib, 3, &npools, &size, NULL, 0) == -1)
        fatal("mbuf: KERN_POOL_NPOOLS failed: %s", strerror(errno));

    for (i = 1; i <= npools; i++) {
        mib[0] = CTL_KERN;
        mib[1] = KERN_POOL;
        mib[2] = KERN_POOL_NAME;
        mib[3] = i;
        size = sizeof(pname);
        if (sysctl(mib, 4, &pname, &size, NULL, 0) == -1)
            continue;

        if (strcmp(pname, "mbufpl") == 0) {
            mbpool_index = i;
            continue;
        }

        if (strncmp(pname, "mcl", 3) != 0)
            continue;

        if (mclpool_count == MCLPOOLS) {
            warning("mbuf: too many mcl* pools");
            break;
        }

        mclpools_index[mclpool_count++] = i;
    }

    if (mclpool_count != MCLPOOLS)
        warning("mbuf: unable to read all %d mcl* pools", MCLPOOLS);

    mib[0] = CTL_KERN;
    mib[1] = KERN_MAXCLUSTERS;
    size = sizeof(maxclusters);
    if (sysctl(mib, 2, &maxclusters, &size, NULL, 0) == -1)
        fatal("mbuf: KERN_MAXCLUSTERS failed: %s", strerror(errno));

    info("started module mbuf(%.200s)", st->arg);
}

int
get_mbuf(char *symon_buf, int maxlen, struct stream *st)
{
    struct mbstat mbstat;
    struct kinfo_pool pool;
    int mib[4];
    size_t size;
    int nmbtypes = sizeof(mbstat.m_mtypes) / sizeof(long);
    int i, totmem, totcnt, totalive, totmbufs, totpct;
    u_int32_t stats[15];

    iter++;

    mib[0] = CTL_KERN;
    mib[1] = KERN_POOL;
    mib[2] = KERN_POOL_POOL;
    mib[3] = mbpool_index;
    size = sizeof(mbpool);

    if (sysctl(mib, 4, &mbpool, &size, NULL, 0) == -1) {
        warning("mbuf: KERN_POOL_POOL mbufpl failed: %s", strerror(errno));
        return 0;
    }
    totmem = mbpool.pr_npages * mbpool.pr_pgsize;
    totcnt = mbpool.pr_npages * mbpool.pr_itemsperpage;
    totalive = mbpool.pr_nget - mbpool.pr_nput;

    /* mbuf cluster counts */
    for (i = 0; i < mclpool_count; i++) {
        mib[3] = mclpools_index[i];
        size = sizeof(pool);

        if (sysctl(mib, 4, &pool, &size, NULL, 0) == -1) {
            warning("mbuf: KERN_POOL_POOL %d failed: %s", i, strerror(errno));
            continue;
        }

        totmem += pool.pr_npages * pool.pr_pgsize;
        totcnt += pool.pr_npages * pool.pr_itemsperpage;
        totalive += pool.pr_nget - pool.pr_nput;
    }

    /*
     * No need to check for configuration changes everytime. Normally this code
     * runs every 5 seconds and the system call is not completely free, so
     * update once every 12 iterations.
     */
    if (iter % 12 == 0) {
        mib[1] = KERN_MAXCLUSTERS;
        size = sizeof(maxclusters);
        if (sysctl(mib, 2, &maxclusters, &size, NULL, 0) == -1)
            warning("mbuf: KERN_MAXCLUSTERS failed: %s", strerror(errno));
    }

    totpct = ((unsigned long)totmem * 100) / ((unsigned long)maxclusters * MCLBYTES);

    mib[1] = KERN_MBSTAT;
    size = sizeof(mbstat);
    if (sysctl(mib, 2, &mbstat, &size, NULL, 0) == -1) {
        warning("mbuf: KERN_MBSTAT failed: %s", strerror(errno));
        return 0;
    }

    totmbufs = 0;
    for (i = 0; i < nmbtypes; ++i)
        totmbufs += mbstat.m_mtypes[i];

    stats[0] = totmbufs;
    stats[1] = mbstat.m_mtypes[MT_DATA];
    stats[2] = mbstat.m_mtypes[MT_OOBDATA];
    stats[3] = mbstat.m_mtypes[MT_CONTROL];
    stats[4] = mbstat.m_mtypes[MT_HEADER];
    stats[5] = mbstat.m_mtypes[MT_FTABLE];
    stats[6] = mbstat.m_mtypes[MT_SONAME];
    stats[7] = mbstat.m_mtypes[MT_SOOPTS];
    stats[8] = totalive;  /* pgused */
    stats[9] = totcnt;    /* pgtotal */
    stats[10] = totmem;
    stats[11] = totpct;
    stats[12] = mbstat.m_drops;
    stats[13] = mbstat.m_wait;
    stats[14] = mbstat.m_drain;

    return snpack(symon_buf, maxlen, st->arg, MT_MBUF,
                  stats[0],
                  stats[1],
                  stats[2],
                  stats[3],
                  stats[4],
                  stats[5],
                  stats[6],
                  stats[7],
                  stats[8],
                  stats[9],
                  stats[10],
                  stats[11],
                  stats[12],
                  stats[13],
                  stats[14]);
}
