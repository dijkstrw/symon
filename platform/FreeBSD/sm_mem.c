/* $Id: sm_mem.c,v 1.3 2005/02/25 15:10:09 dijkstra Exp $ */

/*
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
 * Get current memory statistics in bytes; reports them back in symon_buf as
 *
 * real active : real total : free : [swap used : swap total]
 *
 * This code is not re-entrant.
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

#define pagetob(size) ((size) << me_pageshift)

/* Globals for this module all start with me_ */
static int me_pagesize;
static int me_pageshift;

static char me_vmnswp_mib_str[] = "vm.nswapdev";
static int me_vmnswp_mib_nam[CTL_MAXNAME];
static size_t me_vmnswp_mib_len = 0;

static char me_vmiswp_mib_str[] = "vm.swap_info";
static int me_vmiswp_mib_nam[CTL_MAXNAME];
static size_t me_vmiswp_mib_len = 0;

static int me_vm_mib[] = {CTL_VM, VM_TOTAL};
static long me_stats[5];

static struct vmtotal me_vmtotal;
static size_t me_vmsize = sizeof(struct vmtotal);

/* Prepare mem module for first use */
void
init_mem(char *s)
{
    me_pagesize = sysconf(_SC_PAGESIZE);
    me_pageshift = 0;
    while (me_pagesize > 1) {
	me_pageshift++;
	me_pagesize >>= 1;
    }

    me_vmnswp_mib_len = CTL_MAXNAME;
    if (sysctlnametomib(me_vmnswp_mib_str, me_vmnswp_mib_nam, &me_vmnswp_mib_len) < 0) {
	warning("sysctlnametomib for nswapdev failed");
	me_vmnswp_mib_len = 0;
    }

    me_vmiswp_mib_len = CTL_MAXNAME;
    if (sysctlnametomib(me_vmiswp_mib_str, me_vmiswp_mib_nam, &me_vmiswp_mib_len) < 0) {
	warning("sysctlnametomib for swap_info failed");
	me_vmiswp_mib_len = 0;
    }

    info("started module mem(%.200s)", s);
}
/* Get memory statistics */
int
get_mem(char *symon_buf, int maxlen, char *s)
{
#ifdef HAS_VM_NSWAPDEV
    int i;
    int vmnswp_dat, vmnswp_siz;
#endif

    if (sysctl(me_vm_mib, 2, &me_vmtotal, &me_vmsize, NULL, 0) < 0) {
	warning("%s:%d: sysctl failed", __FILE__, __LINE__);
	bzero(&me_vmtotal, sizeof(me_vmtotal));
    }

    /* convert memory stats to Kbytes */
    me_stats[0] = pagetob(me_vmtotal.t_arm);
    me_stats[1] = pagetob(me_vmtotal.t_rm);
    me_stats[2] = pagetob(me_vmtotal.t_free);

#ifdef HAS_VM_NSWAPDEV
    vmnswp_siz = sizeof (int);
    if (sysctl(me_vmnswp_mib_nam, me_vmnswp_mib_len, &vmnswp_dat, (void *)&vmnswp_siz, NULL, 0) < 0) {
	warning("%s:%d: sysctl nswapdev failed", __FILE__, __LINE__);
	vmnswp_dat = 0;
    }
#endif

    me_stats[3] = me_stats[4] = 0;

#ifdef HAS_VM_NSWAPDEV
    for (i = 0; i < vmnswp_dat; i++) {
	struct xswdev vmiswp_dat;
	int vmiswp_siz;
	me_vmiswp_mib_nam[me_vmiswp_mib_len] = i;
	if (sysctl(me_vmiswp_mib_nam, me_vmiswp_mib_len + 1, &vmiswp_dat, (void *)&vmiswp_siz, NULL, 0) < 0)
		continue;
	me_stats[3] += (vmiswp_dat.xsw_used * DEV_BSIZE);
	me_stats[4] += (vmiswp_dat.xsw_nblks * DEV_BSIZE);
    }
#endif

    return snpack(symon_buf, maxlen, s, MT_MEM,
		  me_stats[0], me_stats[1], me_stats[2],
		  me_stats[3], me_stats[4]);
}
