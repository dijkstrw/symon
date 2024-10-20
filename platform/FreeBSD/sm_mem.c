/*
 * Copyright (c) 2004      Matthew Gream
 * Copyright (c) 2001-2024 Willem Dijkstra
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

#include "conf.h"

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <vm/vm_param.h>

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
static u_int64_t me_pagesize;
static u_int64_t me_pageshift;

static char me_physmem_mib_str[] = "hw.physmem";
static int me_physmem_mib_nam[CTL_MAXNAME];
static size_t me_physmem_mib_len = 0;

static char me_vmnswp_mib_str[] = "vm.nswapdev";
static int me_vmnswp_mib_nam[CTL_MAXNAME];
static size_t me_vmnswp_mib_len = 0;

static char me_vmiswp_mib_str[] = "vm.swap_info";
static int me_vmiswp_mib_nam[CTL_MAXNAME];
static size_t me_vmiswp_mib_len = 0;

static int me_vm_mib[] = {CTL_VM, VM_TOTAL};
static u_int64_t me_stats[5];

static struct vmtotal me_vmtotal;
static size_t me_vmsize = sizeof(struct vmtotal);

void
init_mem(struct stream *st)
{
    me_pagesize = sysconf(_SC_PAGESIZE);
    me_pageshift = 0;
    while (me_pagesize > 1) {
        me_pageshift++;
        me_pagesize >>= 1;
    }

    me_physmem_mib_len = CTL_MAXNAME;
    if (sysctlnametomib(me_physmem_mib_str, me_physmem_mib_nam, &me_physmem_mib_len) < 0) {
        warning("sysctlnametomib for %.200s failed", me_physmem_mib_str);
        me_physmem_mib_len = 0;
    }

    me_vmnswp_mib_len = CTL_MAXNAME;
    if (sysctlnametomib(me_vmnswp_mib_str, me_vmnswp_mib_nam, &me_vmnswp_mib_len) < 0) {
        warning("sysctlnametomib for %.200s failed", me_vmnswp_mib_str);
        me_vmnswp_mib_len = 0;
    }

    me_vmiswp_mib_len = CTL_MAXNAME;
    if (sysctlnametomib(me_vmiswp_mib_str, me_vmiswp_mib_nam, &me_vmiswp_mib_len) < 0) {
        warning("sysctlnametomib for %.200s failed", me_vmiswp_mib_str);
        me_vmiswp_mib_len = 0;
    }

    info("started module mem(%.200s)", st->arg);
}

void
gets_mem(void)
{
#ifdef HAS_XSWDEV
    u_int vmnswp_dat;
    u_long vmnswp_siz;
    u_int i;
#endif
    u_long physmem_dat;
    u_long physmem_siz;

    bzero(&me_vmtotal, sizeof(me_vmtotal));

    if (sysctl(me_vm_mib, 2, &me_vmtotal, &me_vmsize, NULL, 0) < 0) {
        warning("%s:%d: sysctl failed", __FILE__, __LINE__);
        return;
    }

    physmem_siz = sizeof(u_long);
    if (sysctl(me_physmem_mib_nam, me_physmem_mib_len, &physmem_dat, (void *)&physmem_siz, NULL, 0) < 0) {
        warning("%s:%d: sysctl %.200s failed", __FILE__, __LINE__, me_physmem_mib_str);
        return;
    }

    /* convert memory stats to Kbytes */
    me_stats[0] = pagetob(me_vmtotal.t_arm);
    me_stats[1] = ((u_int64_t) physmem_dat) - pagetob(me_vmtotal.t_free);
    me_stats[2] = pagetob(me_vmtotal.t_free);
    me_stats[3] = me_stats[4] = 0;

#ifdef HAS_XSWDEV
    vmnswp_siz = sizeof(int);
    if (sysctl(me_vmnswp_mib_nam, me_vmnswp_mib_len, &vmnswp_dat, (void *)&vmnswp_siz, NULL, 0) < 0) {
        warning("%s:%d: sysctl %.200s failed", __FILE__, __LINE__, me_vmnswp_mib_str);
        vmnswp_dat = 0;
    }
    for (i = 0; i < vmnswp_dat; i++) {
        struct xswdev vmiswp_dat;
        int vmiswp_siz = sizeof(vmiswp_dat);
        me_vmiswp_mib_nam[me_vmiswp_mib_len] = i;
        if (sysctl(me_vmiswp_mib_nam, me_vmiswp_mib_len + 1, &vmiswp_dat, (void *)&vmiswp_siz, NULL, 0) < 0)
                continue;
        me_stats[3] += pagetob(vmiswp_dat.xsw_used);
        me_stats[4] += pagetob(vmiswp_dat.xsw_nblks);
    }
#endif
}

int
get_mem(char *symon_buf, int maxlen, struct stream *st)
{
    return snpack(symon_buf, maxlen, st->arg, MT_MEM2,
                  me_stats[0], me_stats[1], me_stats[2],
                  me_stats[3], me_stats[4]);
}
