/* $Id: symonnet.c,v 1.4 2002/03/31 14:27:47 dijkstra Exp $ */

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

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <string.h>
#include <errno.h>
#include <time.h>

#include "error.h"
#include "data.h"
#include "mon.h"

/* Fill a mux structure with inet details */
void 
connect2mux(struct mux *mux)
{
    struct sockaddr_in sockaddr;

    if ((mux->socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
	fatal("Could not obtain socket: %.200s", strerror(errno));

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&sockaddr.sin_zero, 8);

    if (bind(mux->socket, (struct sockaddr *) &sockaddr, sizeof(struct sockaddr)) == -1)
	fatal("Could not bind socket: %.200s", strerror(errno));

    mux->sockaddr.sin_family = AF_INET;
    mux->sockaddr.sin_port = htons(mux->port);
    mux->sockaddr.sin_addr.s_addr = htonl(mux->ip);
    bzero(&mux->sockaddr.sin_zero, 8);
}
/* Send data stored in the mux structure to a mux */
void 
send_packet(struct mux *mux)
{   
    if (sendto(mux->socket, (void *)&mux->packet, 
	       mux->offset + sizeof(mux->packet.header), 0,
	       (struct sockaddr *)&mux->sockaddr, sizeof(mux->sockaddr)) 
	!= (mux->offset + sizeof(mux->packet.header))) {
	mux->senderr++;
    }

    if (mux->senderr >= MON_WARN_SENDERR)
	warning("%d updates to mux(%u.%u.%u.%u) lost due to send errors",
		mux->senderr, 
		(mux->ip >> 24), (mux->ip >> 16) & 0xff, 
		(mux->ip >> 8) & 0xff, mux->ip & 0xff), mux->senderr = 0;
}
/* Prepare a packet for data */
void 
prepare_packet(struct mux *mux) 
{
    time_t t = time(NULL);

    memset(&mux->packet, 0, sizeof(mux->packet));
    mux->offset = 0;

    mux->packet.header.mon_version = MON_PACKET_VER;
    mux->packet.header.timestamp = htonq((u_int64_t) t);
}
/* Put a stream into the packet for a mux */
void 
stream_in_packet(struct stream *stream, struct mux *mux) 
{
    mux->offset += 
	(streamfunc[stream->type].get)      /* call getter of stream */
	(&mux->packet.data[mux->offset],    /* packet buffer */
	 _POSIX2_LINE_MAX - mux->offset,    /* maxlen */
	 stream->args);
}
/* Wrap up packet for sending */
void finish_packet(struct mux *mux) 
{
    mux->packet.header.length = htons(mux->offset);
    mux->packet.header.crc = 0;
}
