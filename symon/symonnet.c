/*
 * $Id: symonnet.c,v 1.1 2002/03/17 13:37:31 dijkstra Exp $
 *
 * Holds all network functions for mon
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <string.h>

#include "error.h"
#include "data.h"
#include "mon.h"

/* connect to a mux */
void connect2mux(struct mux *mux) {
    struct sockaddr_in sockaddr;

    if ((mux->socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	fatal("Could not obtain socket.");
    }

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&sockaddr.sin_zero, 8);

    if (bind(mux->socket, (struct sockaddr *) &sockaddr, sizeof(struct sockaddr)) == -1) {
	fatal("Could not bind socket.");
    }

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(mux->port);
    sockaddr.sin_addr.s_addr = htonl(mux->ip);
    bzero(&sockaddr.sin_zero, 8);

    if (connect(mux->socket, (struct sockaddr *) &sockaddr, sizeof(struct sockaddr)) == -1) {
	fatal("Could not connect to %u.%u.%u.%u", 
	      (mux->ip >> 24), (mux->ip >> 16) & 0xff, 
	      (mux->ip >> 8) & 0xff, mux->ip & 0xff);
    }
}

/* send data stored in the mux structure to a mux */
void senddata(struct mux *mux) {   

    if (send(mux->socket, mux->data, mux->offset, 0) != mux->offset) {
	mux->senderr++;
    }

    if (mux->senderr >= MON_WARN_SENDERR) {
	syslog(LOG_INFO, "%d updates to mux(%u.%u.%u.%u) lost due to send errors",
	       mux->senderr, 
	       (mux->ip >> 24), (mux->ip >> 16) & 0xff, 
	       (mux->ip >> 8) & 0xff, mux->ip & 0xff);

	mux->senderr = 0;
    }
}
