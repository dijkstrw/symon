/*
 * $Id: sm_if.c,v 1.1 2002/03/09 16:25:33 dijkstra Exp $
 *
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

#include <string.h>
#include <sys/types.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <syslog.h>
#include <varargs.h>
#include <limits.h>
/* Include all possible interfaces => need to know struct ifnet sizes */
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
#include "mon.h"

struct ifnet ne_ifnet;
size_t ne_size_ifnet;
struct ifnet_head ne_ifhead;       /* TAILQ_HEAD */
size_t ne_size_ifhead;
union {
    struct ifaddr   ifa;
    struct in_ifaddr in;
    struct in6_ifaddr in6;
    struct ns_ifaddr ns;
    struct ipx_ifaddr ipx;
    struct iso_ifaddr iso;
} ne_ifaddr;
size_t ne_size_ifaddr;
char ne_name[IFNAMSIZ];

void init_if(s) 
    char *s;
{
    ne_size_ifnet = sizeof ne_ifnet;
    ne_size_ifhead = sizeof ne_ifhead;
    ne_size_ifaddr = sizeof ne_ifaddr;
}
int get_if(mon_buf, maxlen, interface) 
    char *mon_buf;
    int maxlen;
    char *interface;
{
    u_long ifnetaddr;

    /* Read interface list */
    ifnetaddr=mon_nl[MON_IFNET].n_value;
    if (ifnetaddr == 0) {
	syslog(LOG_ALERT,"ifstat.c:%d: ifnet = symbol not defined",__LINE__);
	exit(1);
    }  
    /*
     * Find the pointer to the first ifnet structure.  Replace
     * the pointer to the TAILQ_HEAD with the actual pointer
     * to the first list element.
     */
    kread(ifnetaddr, (char *) &ne_ifhead, ne_size_ifhead);
    ifnetaddr = (u_long) ne_ifhead.tqh_first;
  
    while (ifnetaddr) {
	kread(ifnetaddr, (char *) &ne_ifnet, ne_size_ifnet);
	bcopy(ne_ifnet.if_xname, (char *) &ne_name, IFNAMSIZ);
	ne_name[IFNAMSIZ - 1] = '\0'; /* sanity */
	ifnetaddr = (u_long) ne_ifnet.if_list.tqe_next;
	if (interface != 0 && strcmp(ne_name, interface) == 0) {
	    return snpack(mon_buf, maxlen, interface, MT_IF,
		      ne_ifnet.if_ipackets, ne_ifnet.if_opackets, 
		      ne_ifnet.if_ibytes, ne_ifnet.if_obytes, 
		      ne_ifnet.if_imcasts, ne_ifnet.if_omcasts, 
		      ne_ifnet.if_ierrors, ne_ifnet.if_oerrors, 
		      ne_ifnet.if_collisions, ne_ifnet.if_iqdrops);
	}
    }
    return 0;
}

