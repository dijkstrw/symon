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
 * Get current memory statistics in bytes; reports them back in symon_buf as
 *
 * real active : real total : free : [swap used : swap total]
 */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/swap.h>
#include <sys/vmmeter.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "error.h"
#include "symon.h"
#include "xmalloc.h"

#define pagetob(size) (((u_int64_t)size) << me_pageshift)

/* Globals for this module all start with me_ */
static u_int64_t me_pageshift;
static u_int64_t me_stats[5];
static int me_vm_mib[] = {CTL_VM, VM_METER};
static struct vmtotal me_vmtotal;
static size_t me_vmsize;
static u_int64_t me_pagesize;
static u_int64_t me_nswap;
struct swapent *me_swdev = NULL;

void
init_mem(struct stream *st)
{
    me_pagesize = sysconf(_SC_PAGESIZE);
    me_pageshift = 0;
    while (me_pagesize > 1) {
        me_pageshift++;
        me_pagesize >>= 1;
    }

    /* get total -- systemwide main memory usage structure */
    me_vmsize = sizeof(me_vmtotal);

    /* determine number of swap entries */
    me_nswap = swapctl(SWAP_NSWAP, 0, 0);

    if (me_swdev) {
        xfree(me_swdev);
    }

    if (me_nswap != 0) {
        me_swdev = xmalloc(me_nswap * sizeof(*me_swdev));
    } else {
        me_swdev = NULL;
    }

    if (me_swdev == NULL && me_nswap != 0) {
        me_nswap = 0;
    }

    if (st != NULL) {
        info("started module mem(%.200s)", st->arg);
    }
}

void
gets_mem()
{
    int i, rnswap;

    if (sysctl(me_vm_mib, 2, &me_vmtotal, &me_vmsize, NULL, 0) < 0) {
        warning("%s:%d: sysctl failed", __FILE__, __LINE__);
        bzero(&me_vmtotal, sizeof(me_vmtotal));
    }

    /* convert memory stats to Kbytes */
    me_stats[0] = pagetob(me_vmtotal.t_arm);
    me_stats[1] = pagetob(me_vmtotal.t_rm);
    me_stats[2] = pagetob(me_vmtotal.t_free);

    rnswap = swapctl(SWAP_STATS, me_swdev, me_nswap);
    if (rnswap == -1) {
        /* A swap device may have been added; increase and retry */
        init_mem(NULL);
        rnswap = swapctl(SWAP_STATS, me_swdev, me_nswap);
    }

    me_stats[3] = me_stats[4] = 0;
    if (rnswap == me_nswap) {   /* Read swap successfully */
        /* Total things up */
        for (i = 0; i < me_nswap; i++) {
            if (me_swdev[i].se_flags & SWF_ENABLE) {
                me_stats[3] += ((u_int64_t)me_swdev[i].se_inuse * DEV_BSIZE);
                me_stats[4] += ((u_int64_t)me_swdev[i].se_nblks * DEV_BSIZE);
            }
        }
    }
}

int
get_mem(char *symon_buf, int maxlen, struct stream *st)
{
    return snpack(symon_buf, maxlen, st->arg, MT_MEM2,
                  me_stats[0], me_stats[1], me_stats[2],
                  me_stats[3], me_stats[4]);
}
