/*
 * $Id: net.h,v 1.3 2002/03/09 16:18:18 dijkstra Exp $
 */

#ifndef _NET_H
#define _NET_H

#include <sys/cdefs.h>
#include <sys/types.h>

extern char lookup_hostname[];
extern char lookup_address[];
extern u_int32_t lookup_ip;

__BEGIN_DECLS
int lookup __P((char *));
__END_DECLS

#endif /* _NET_H */
