/* $Id: sm_if.c,v 1.1 2005/01/14 16:12:55 dijkstra Exp $ */

/*
 * Copyright (c) 2005 Fredrik Soderblom
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

/* Globals for this module start with if_ */
static int if_s = -1;
/* Prepare if module for first use */
void
init_if(char *s)
{
    if (if_s == -1)
	if ((if_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	    fatal("%s:%d: socket failed, %.200",
		  __FILE__, __LINE__, strerror(errno));

    info("started module if(%.200s)", s);
}

int
get_ifcount(void)
{
    int name[5], count;
    size_t len;
  
    name[0] = CTL_NET;
    name[1] = PF_LINK;
    name[2] = NETLINK_GENERIC;
    name[3] = IFMIB_SYSTEM;
    name[4] = IFMIB_IFCOUNT;

    len = sizeof(int);

    if (sysctl(name, 5, &count, &len, NULL, 0) != -1)
        return(count);
    else
        return(-1);
}

int
get_ifmib_general(int row, struct ifmibdata *ifmd)
{
    int name[6];
    size_t len;

    name[0] = CTL_NET;
    name[1] = PF_LINK;
    name[2] = NETLINK_GENERIC;
    name[3] = IFMIB_IFDATA;
    name[4] = row;
    name[5] = IFDATA_GENERAL;

    len = sizeof(*ifmd);

    return sysctl(name, 6, ifmd, &len, (void *)0, 0);
}

/* Get interface statistics */
void
gets_if()
{
}
int
get_if(char *symon_buf, int maxlen, char *interface)
{
    int i;
    struct ifmibdata ifmd;
    struct if_data ifdata;
    int ifcount = get_ifcount();

    for (i = 1; i <= ifcount; i++) {
        get_ifmib_general(i, &ifmd);
        if (!strcmp(ifmd.ifmd_name, interface))
            break;
    }

    ifdata = ifmd.ifmd_data;
    return snpack(symon_buf, maxlen, interface, MT_IF,
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
