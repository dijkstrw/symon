/*
 * $Id: net.h,v 1.2 2001/09/20 19:26:33 dijkstra Exp $
 */

#ifndef _NET_H
#define _NET_H

#include <sys/types.h>

extern char lookup_hostname[];
extern char lookup_address[];
extern u_int32_t lookup_ip;

int lookup(char *);

#endif /* _NET_H */
