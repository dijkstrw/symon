/* $Id: sm_if.c,v 1.1 2004/08/07 12:21:36 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2004 Willem Dijkstra
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

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netns/ns.h>
#include <netns/ns_if.h>
#include <netipx/ipx.h>
#include <netipx/ipx_if.h>

#include <errno.h>
#include <limits.h>
#include <string.h>

#include "error.h"
#include "symon.h"

/* Prepare if module for first use */
void
init_if(char *s)
{
    info("started module if(%.200s)", s);
}
void
gets_if()
{
    char buf[SYMON_MAX_OBJSIZE];
    char *bptr;
    int readsize;
    int if_s;

    if ((if_s = open("/proc/net/dev", O_RDONLY)) < 0) {
	warning("cannot access /proc/net/dev: %.200s", strerror(errno));
	return;
    }

    bzero(&buf, sizeof(buf));

    read(if_s, &buf, 

    if (
/* Get interface statistics */
int
get_if(char *symon_buf, int maxlen, char *interface)
{
    struct ifreq ifr;
    struct if_data ifdata;

    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    ifr.ifr_data = (caddr_t) & ifdata;

    if (ioctl(if_s, SIOCGIFDATA, &ifr)) {
	warning("if(%.200s) failed (ioctl error)", interface);
	return 0;
    }

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