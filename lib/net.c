/* $Id: net.c,v 1.5 2002/03/31 14:27:46 dijkstra Exp $ */

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

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/types.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

/*
 * lookup( hostname ) - hostname resolver
 *
 * Lookup returns 1 if hostname could be resolved. Resolved data is
 * stored in the globals lookup_hostname, lookup_address and lookup_ip below.  
 */
char lookup_hostname[_POSIX2_LINE_MAX];
char lookup_address[_POSIX2_LINE_MAX];
u_int32_t lookup_ip;

int 
lookup(char *name)
{
    struct in_addr addr;
    struct hostent *host;
    extern int h_errno;
    char *chostname, *sptr;
    int i, bestl, newl;
	
    strcpy(lookup_hostname, "unresolved.");
    strcpy(lookup_address, "unresolved.");
    strncat(lookup_hostname, name, (_POSIX2_LINE_MAX - 1 - sizeof("unresolved")));
    strncat(lookup_address, name, (_POSIX2_LINE_MAX - 1 - sizeof("unresolved")));

    if (4 == sscanf(name, "%4d.%4d.%4d.%4d", &i, &i, &i, &i)) {
	addr.s_addr = inet_addr(name);
	if (addr.s_addr == 0xffffffff) 
	    return 0;
	    
	host = gethostbyaddr((char *)&addr.s_addr, 4, AF_INET);
    } else {
	host = gethostbyname(name);
    }
    
    if (host == NULL) {
	return 0;
    } else {
	i = 0;
	newl = 0;
	bestl = 0;
	sptr = (char *)(host->h_name ? host->h_name : host->h_aliases[0]);
	chostname = NULL;

	while (sptr) {
	    newl = strlen(sptr);
	    if (newl > bestl) {
		bestl = newl;
		chostname = sptr;
	    }

	    sptr = host->h_aliases[i++];
	}
	
	if (chostname)
	    snprintf(lookup_hostname, (_POSIX2_LINE_MAX - 1), "%s", chostname);
	
	if (*host->h_addr_list) {
	    lookup_ip = ntohl(*(unsigned long *) *(char **) host->h_addr_list);

	    snprintf(lookup_address, (_POSIX2_LINE_MAX - 1),"%u.%u.%u.%u", 
		    (lookup_ip >> 24), (lookup_ip >> 16) & 0xff,
		    (lookup_ip >> 8) & 0xff, lookup_ip & 0xff);
	}
    }

    return 1;
}
