/* $Id: sm_if.c,v 1.3 2002/04/01 20:15:59 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2002 Willem Dijkstra
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
 * Get current interface statistics from kernel and return them in mon_buf as
 *
 * ipackets : opackets : ibytes : obytes : imcasts : omcasts : ierrors : oerrors : colls : drops
 *
 * This module uses the kvm interface to read kernel values directly. It needs
 * a valid kvm handle and will only work if this has been obtained by someone
 * belonging to group kmem. Note that these priviledges (sgid kmem) can be
 * dropped right after the handle has been obtained.  
 * 
 * Re-entrant code: globals are used to speedup calculations for all interfaces.
 */

#include <sys/types.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netns/ns.h>
#include <netns/ns_if.h>
#include <netipx/ipx.h>
#include <netipx/ipx_if.h>
#include <netiso/iso.h>
#include <netiso/iso_var.h>

#include <string.h>
#include <limits.h>

#include "error.h"
#include "mon.h"

/* Globals for this module start with if_ */
struct ifnet if_ifnet;
size_t if_size_ifnet;
struct ifnet_head if_ifhead;   
size_t if_size_ifhead;
union {
    struct ifaddr   ifa;
    struct in_ifaddr in;
    struct in6_ifaddr in6;
    struct ns_ifaddr ns;
    struct ipx_ifaddr ipx;
    struct iso_ifaddr iso;
} if_ifaddr;
size_t if_size_ifaddr;
char if_name[IFNAMSIZ];
/* Prepare if module for first use */
void 
init_if(char *s) 
{
    if_size_ifnet = sizeof if_ifnet;
    if_size_ifhead = sizeof if_ifhead;
    if_size_ifaddr = sizeof if_ifaddr;

    info("started module if(%s)", s);
}
/* Get interface statistics */
int 
get_if(char *mon_buf, int maxlen, char *interface) 
{
    u_long ifnetptr;

    ifnetptr = mon_nl[MON_IFNET].n_value;
    if (ifnetptr == 0)
	fatal("%s:%d: ifnet = symbol not defined", __FILE__, __LINE__);

    /* obtain first ifnet structure in kernel memory */
    kread(ifnetptr, (char *) &if_ifhead, if_size_ifhead);
    ifnetptr = (u_long) if_ifhead.tqh_first;
  
    while (ifnetptr) {
	kread(ifnetptr, (char *) &if_ifnet, if_size_ifnet);
	bcopy(if_ifnet.if_xname, (char *) &if_name, IFNAMSIZ);
	if_name[IFNAMSIZ - 1] = '\0';
	ifnetptr = (u_long) if_ifnet.if_list.tqe_next;

	if (interface != 0 && strncmp(if_name, interface, IFNAMSIZ) == 0) {
	    return snpack(mon_buf, maxlen, interface, MT_IF,
		      if_ifnet.if_ipackets, if_ifnet.if_opackets, 
		      if_ifnet.if_ibytes, if_ifnet.if_obytes, 
		      if_ifnet.if_imcasts, if_ifnet.if_omcasts, 
		      if_ifnet.if_ierrors, if_ifnet.if_oerrors, 
		      if_ifnet.if_collisions, if_ifnet.if_iqdrops);
	}
    }

    return 0;
}

