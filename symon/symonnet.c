/*
 * $Id: symonnet.c,v 1.3 2002/03/29 15:17:08 dijkstra Exp $
 *
 * Holds all network functions for mon
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "error.h"
#include "data.h"
#include "mon.h"

/* connect to a mux */
void connect2mux(mux)
    struct mux *mux;
{
    struct sockaddr_in sockaddr;

    if ((mux->socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	fatal("Could not obtain socket: %.200s", strerror(errno));
    }

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&sockaddr.sin_zero, 8);

    if (bind(mux->socket, (struct sockaddr *) &sockaddr, sizeof(struct sockaddr)) == -1) {
	fatal("Could not bind socket: %.200s", strerror(errno));
    }

    mux->sockaddr.sin_family = AF_INET;
    mux->sockaddr.sin_port = htons(mux->port);
    mux->sockaddr.sin_addr.s_addr = htonl(mux->ip);
    bzero(&mux->sockaddr.sin_zero, 8);
}

/* send data stored in the mux structure to a mux */
void send_packet(mux)
    struct mux *mux;
{   
    if (sendto(mux->socket, (void *)&mux->packet, 
	       mux->offset + sizeof(mux->packet.header), 0,
	       (struct sockaddr *)&mux->sockaddr, sizeof(mux->sockaddr)) 
	!= mux->offset) {
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

/* prepare a packet for data */
void prepare_packet(mux) 
    struct mux *mux;
{
    time_t t = time(NULL);

    memset(&mux->packet, 0, sizeof(mux->packet));
    mux->offset = 0;

    mux->packet.header.mon_version = MON_STREAMVER;
    mux->packet.header.timestamp = htonq((u_int64_t) t);
}

/* put a stream into the packet for a mux */
void stream_in_packet(stream, mux) 
    struct stream *stream;
    struct mux* mux;
{
    mux->offset += 
	(streamfunc[stream->type].get)      /* call getter of stream */
	(&mux->packet.data[mux->offset],    /* packet buffer */
	 _POSIX2_LINE_MAX - mux->offset,    /* maxlen */
	 stream->args);
}

/* wrap up packet for sending */
void finish_packet(mux) 
    struct mux* mux;
{
    mux->packet.header.length = htons(mux->offset);
    mux->packet.header.crc = 0;
}
