/* $Id: sm_mbuf.c,v 1.1 2004/08/07 12:21:36 dijkstra Exp $ */

/*
 * Copyright (c) 2004 Matthew Gream
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

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>

#include "error.h"
#include "symon.h"

static char mbstat_mib_str[] = "kern.ipc.mbstat";
static int mbstat_mib[CTL_MAXNAME];
static size_t mbstat_len = 0;

/* Prepare if module for first use */
void
init_mbuf(char *s)
{
    mbstat_len = CTL_MAXNAME;
    if (sysctlnametomib(mbstat_mib_str, mbstat_mib, &mbstat_len) < 0) {
	warning("sysctlnametomib for mbuf failed");
	mbstat_len = 0;
    }

    info("started module mbuf(%.200s)", s);
}

/* Get mbuf statistics */
int
get_mbuf(char *symon_buf, int maxlen, char *arg)
{
    struct mbstat mbstat;
#ifdef KERN_POOL
    int npools;
    struct pool pool, mbpool, mclpool;
    char name[32];
    int flag = 0;
    int page_size = getpagesize();
#endif
    size_t size;
    int totmem, totused, totmbufs, totpct;
    u_int32_t stats[15];

    if (!mbstat_len)
	return 0;

    size = sizeof(mbstat);
    if (sysctl(mbstat_mib, mbstat_len, &mbstat, &size, NULL, 0) < 0) {
	warning("mbuf(%.200s) failed (sysctl() %.200s)", arg, strerror(errno));
	return (0);
    }

#ifdef KERN_POOL
    mib[0] = CTL_KERN;
    mib[1] = KERN_POOL;
    mib[2] = KERN_POOL_NPOOLS;
    size = sizeof(npools);
    if (sysctl(mib, 3, &npools, &size, NULL, 0) < 0) {
	warning("mbuf(%.200s) failed (sysctl() %.200s)", arg, strerror(errno));
	return (0);
    }

    for (i = 1; npools; ++i) {
	mib[0] = CTL_KERN;
	mib[1] = KERN_POOL;
	mib[2] = KERN_POOL_POOL;
	mib[3] = i;
	size = sizeof(pool);
	if (sysctl(mib, 4, &pool, &size, NULL, 0) < 0) {
	    warning("mbuf(%.200s) failed (sysctl() %.200s)", arg, strerror(errno));
	    return (0);
	}
	npools--;
	mib[2] = KERN_POOL_NAME;
	size = sizeof(name);
	if (sysctl(mib, 4, name, &size, NULL, 0) < 0) {
	    warning("mbuf(%.200s) failed (sysctl() %.200s)", arg, strerror(errno));
	    return (0);
	}
	if (!strcmp(name, "mbpl")) {
	    bcopy(&pool, &mbpool, sizeof(pool));
	    flag |= 1;
	} else if (!strcmp(name, "mclpl")) {
	    bcopy(&pool, &mclpool, sizeof(pool));
	    flag |= 2;
	}
	if (flag == 3)
	    break;
    }
    if (flag != 3) {
	warning("mbuf(%.200s) failed (flag != 3)", arg);
	return (0);
    }
#endif

    totmbufs = 0; /* XXX: collect mb_statpcpu and add together */
#ifdef KERN_POOL
    totmem = (mbpool.pr_npages + mclpool.pr_npages) * page_size;
    totused = (mbpool.pr_nget - mbpool.pr_nput) * mbpool.pr_size +
	(mclpool.pr_nget - mclpool.pr_nput) * mclpool.pr_size;
#else
    totmem = 0;
    totused = 0;
#endif
    totpct = (totmem == 0) ? 0 : ((totused * 100) / totmem);

    stats[0] = totmbufs;
    stats[1] = 0; /*mbstat.m_mtypes[MT_DATA];*/
    stats[2] = 0; /*mbstat.m_mtypes[MT_OOBDATA];*/
    stats[3] = 0; /*mbstat.m_mtypes[MT_CONTROL];*/
    stats[4] = 0; /*mbstat.m_mtypes[MT_HEADER];*/
    stats[5] = 0; /*mbstat.m_mtypes[MT_FTABLE];*/
    stats[6] = 0; /*mbstat.m_mtypes[MT_SONAME];*/
    stats[7] = 0; /*mbstat.m_mtypes[MT_SOOPTS];*/
#ifdef KERN_POOL
    stats[8] = mclpool.pr_nget - mclpool.pr_nput;
    stats[9] = mclpool.pr_npages * mclpool.pr_itemsperpage;
#else
    stats[8] = 0;
    stats[9] = 0;
#endif
    stats[10] = totmem;
    stats[11] = totpct;
    stats[12] = mbstat.m_drops;
    stats[13] = mbstat.m_wait;
    stats[14] = mbstat.m_drain;

    return snpack(symon_buf, maxlen, arg, MT_MBUF,
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
