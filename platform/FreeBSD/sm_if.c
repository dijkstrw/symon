/* $Id: sm_if.c,v 1.2 2005/10/16 15:26:54 dijkstra Exp $ */

/*
 * Copyright (c) 2005 Fredrik Soderblom
 * Copyright (c) 2005 Willem Dijkstra
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
 * Get current interface statistics from kernel and return them in symon_buf as
 *
 * ipackets : opackets : ibytes : obytes : imcasts : omcasts : ierrors :
 * oerrors : colls : drops
 *
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_mib.h>

#include <errno.h>
#include <limits.h>
#include <string.h>

#include "error.h"
#include "symon.h"
#include "xmalloc.h"

/* Globals for this module start with if_ */
static int if_cur = 0;
static int if_max = 0;
struct ifmibdata *if_md = NULL;

/* Prepare if module for first use */
void
init_if(struct stream *st)
{
    info("started module if(%.200s)", st->arg);
}

int
get_ifcount(void)
{
    int name[5] = {CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_SYSTEM, IFMIB_IFCOUNT};
    int count;
    size_t len;

    len = sizeof(int);

    if (sysctl(name, 5, &count, &len, NULL, 0) != -1) {
	return count;
    } else {
	return -1;
    }
}

int
get_ifmib_general(int row, struct ifmibdata *ifmd)
{
    int name[6] = {CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_IFDATA, row, IFDATA_GENERAL};
    size_t len;

    len = sizeof(*ifmd);

    return sysctl(name, 6, ifmd, &len, (void *)0, 0);
}

/* Get interface statistics */
void
gets_if()
{
    int i;

    /* how much memory is needed */
    if_cur = get_ifcount();

    /* increase buffers if necessary */
    if (if_cur > if_max) {
	if_max = if_cur;

	if (if_max > SYMON_MAX_DOBJECTS) {
	    fatal("%s:%d: dynamic object limit (%d) exceeded for ifmibdata structures",
		  __FILE__, __LINE__, SYMON_MAX_DOBJECTS);
	}

	if_md = xrealloc(if_md, if_max * sizeof(struct ifmibdata));
    }

    /* read data */
    for (i = 1; i <= if_cur; i++) {
	get_ifmib_general(i, &if_md[i - 1]);
    }
}
int
get_if(char *symon_buf, int maxlen, struct stream *st)
{
    int i;
    struct if_data ifdata;

    for (i = 1; i <= if_cur; i++) {
	if (!strcmp(if_md[i - 1].ifmd_name, st->arg)) {
	    ifdata = if_md[i - 1].ifmd_data;
	    return snpack(symon_buf, maxlen, st->arg, MT_IF,
			  ifdata.ifi_ipackets,
			  ifdata.ifi_opackets,
			  ifdata.ifi_ibytes,
			  ifdata.ifi_obytes,
			  ifdata.ifi_imcasts,
			  ifdata.ifi_omcasts,
			  ifdata.ifi_ierrors,
			  ifdata.ifi_oerrors,
			  ifdata.ifi_collisions,
			  ifdata.ifi_iqdrops);
	}
    }

    return 0;
}
