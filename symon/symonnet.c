/* $Id: symonnet.c,v 1.10 2002/09/14 15:49:39 dijkstra Exp $ */

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
#include "symon.h"
#include "net.h"

/* Fill a mux structure with inet details */
void 
connect2mux(struct mux *mux)
{
    struct sockaddr_in sockaddr;

    if ((mux->symonsocket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	fatal("could not obtain socket: %.200s", strerror(errno));

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = 0;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&sockaddr.sin_zero, 8);

    if (bind(mux->symonsocket, (struct sockaddr *) &sockaddr, 
	     sizeof(struct sockaddr)) == -1)
	fatal("could not bind socket: %.200s", strerror(errno));

    mux->sockaddr.sin_family = AF_INET;
    mux->sockaddr.sin_port = htons(mux->port);
    mux->sockaddr.sin_addr.s_addr = htonl(mux->ip);
    bzero(&mux->sockaddr.sin_zero, 8);

    info("sending packets to udp:%s:%d", 
	 mux->name, mux->port);
}
/* Send data stored in the mux structure to a mux */
void 
send_packet(struct mux *mux)
{   
    if (sendto(mux->symonsocket, (void *)&mux->packet.data, 
	       mux->offset, 0, (struct sockaddr *)&mux->sockaddr, 
	       sizeof(mux->sockaddr))
	!= mux->offset) {
	mux->senderr++;
    }

    if (mux->senderr >= SYMON_WARN_SENDERR)
	warning("%d updates to mux(%u.%u.%u.%u) lost due to send errors",
		mux->senderr, 
		IPAS4BYTES(mux->ip)), mux->senderr = 0;
}
/* Prepare a packet for data */
void 
prepare_packet(struct mux *mux) 
{
    time_t t = time(NULL);

    bzero(&mux->packet, sizeof(mux->packet));
    mux->packet.header.symon_version = SYMON_PACKET_VER;
    mux->packet.header.timestamp = t;

    /* symonpacketheader is always first stream */
    mux->offset = 
	setheader((char *)&mux->packet.data, 
		  &mux->packet.header);
}
/* Put a stream into the packet for a mux */
void 
stream_in_packet(struct stream *stream, struct mux *mux) 
{
    mux->offset += 
	(streamfunc[stream->type].get)      /* call getter of stream */
	(&mux->packet.data[mux->offset],    /* packet buffer */
	 sizeof(mux->packet.data) - mux->offset,    /* maxlen */
	 stream->args);
}
/* Ready a packet for transmission, set length and crc */
void finish_packet(struct mux *mux) 
{
    mux->packet.header.length = mux->offset;
    mux->packet.header.crc = 0;

    /* fill in correct header with crc = 0 */
    setheader((char *)&mux->packet.data, &mux->packet.header);

    /* fill in correct header with crc */
    mux->packet.header.crc = crc32(&mux->packet.data, mux->offset);
    setheader((char *)&mux->packet.data, &mux->packet.header);
}

