/*
 * $Id: net.c,v 1.1 2001/09/02 18:59:24 dijkstra Exp $
 *
 * Holds all network functions for monmux
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

/*
 * lookup( hostname )
 *
 * returns 1 if hostname could be resolved. Data will be stored in the globals
 * below.  
 */
char lookup_hostname[_POSIX2_LINE_MAX];
char lookup_address[_POSIX2_LINE_MAX];

int lookup(name)
    char *name;
{
    int j, by_name;
    struct hostent *host;
    extern int h_errno;
    struct in_addr addr;	/* address in host order */
    
    strcpy(lookup_hostname, "unresolved.");
    strcpy(lookup_address, "unresolved.");
    strcat(lookup_hostname, name);
    strcat(lookup_address, name);

    /* does the argument look like an address? */
    
    if (4 == sscanf(name, "%4d.%4d.%4d.%4d", &j, &j, &j, &j)) {
	/* ip */
	addr.s_addr = inet_addr(name);
	if (addr.s_addr == 0xffffffff) 
	    return 0;
	    
	host = gethostbyaddr((char *)&addr.s_addr, 4, AF_INET);
	by_name = 0;
    } else {
	/* name */
	host = gethostbyname(name);
	by_name = 1;
    }
    
    if (host == NULL) {
	/* gethostxxx error */
	return 0;
    } else {
	int i = 0, bestl = 0;	/* best known fit */
	int newl = 0;	/* new length */
	char *sptr;
	char *chostname = NULL;
	
	sptr = (char *) (host->h_name ? host->h_name : host->h_aliases[0]);
	
	while (sptr) {
	    newl = strlen(sptr);
	    if (newl > bestl) {
		bestl = newl;
		chostname = sptr;
	    }
	    sptr = host->h_aliases[i++];
	}
	
	if (chostname) {
	    snprintf(lookup_hostname, _POSIX2_LINE_MAX-1, "%s", chostname);
	}
	if (*host->h_addr_list) {
	    unsigned long   horder;
	    
	    horder = ntohl(*(unsigned long *) *(char **) host->h_addr_list);
	    sprintf(lookup_address, "%lu.%lu.%lu.%lu", 
		    (horder >> 24), (horder >> 16) & 0xff,
		    (horder >> 8) & 0xff, horder & 0xff);
	}
    }

    return 1;
}
