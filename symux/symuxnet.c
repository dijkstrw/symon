/* $Id: symuxnet.c,v 1.8 2002/08/11 20:00:41 dijkstra Exp $ */

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

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "data.h"
#include "error.h"
#include "monmux.h"
#include "muxnet.h"
#include "net.h"
#include "xmalloc.h"
#include "share.h"

/* Obtain a udp socket for incoming mon traffic */
int 
getmonsocket(struct mux *mux) 
{
    struct sockaddr_in sockaddr;
    int sock;
    
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
	fatal("could not obtain socket: %.200s", strerror(errno));

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(mux->port);
    sockaddr.sin_addr.s_addr = htonl(mux->ip);
    bzero(&sockaddr.sin_zero, sizeof(sockaddr.sin_zero));

    if (bind(sock, (struct sockaddr *) &sockaddr, 
	     sizeof(struct sockaddr)) == -1)
	fatal("could not bind socket: %.200s", strerror(errno));

    mux->monsocket = sock;

    info("listening for incoming mon traffic on udp:%s:%d", 
	 mux->name, mux->port);

    return sock;
}
/* Obtain a listen socket for new clients */
int
getclientsocket(struct mux *mux)
{
  struct sockaddr_in sockaddr;
  int sock;
  
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	fatal("could not obtain socket: %.200s", strerror(errno));

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(mux->port);
    sockaddr.sin_addr.s_addr = htonl(mux->ip);
    bzero(&sockaddr.sin_zero, sizeof(sockaddr.sin_zero));

    if (bind(sock, (struct sockaddr *) &sockaddr, 
	     sizeof(struct sockaddr)) == -1)
	fatal("could not bind socket: %.200s", strerror(errno));

    if (listen(sock, MONMUX_TCPBACKLOG) == -1)
      fatal("could not listen to socket: %.200s", strerror(errno));

    fcntl(sock, O_NONBLOCK);
    mux->clientsocket = sock;

    info("listening for incoming connections on tcp:%s:%d", 
	 mux->name, mux->port);

    return sock;
}
/*
 * Wait for traffic (mon reports from a source in sourclist | clients trying to connect
 * Returns the <source> and <packet>
 * Silently forks off clienthandlers
 */
void
waitfortraffic(struct mux *mux, struct sourcelist *sourcelist, 
	       struct source **source, struct monpacket *packet) 
{
    fd_set readset;
    int socksactive;
    int maxsock;

    maxsock = ((mux->clientsocket > mux->monsocket) ?
	       mux->clientsocket :
	       mux->monsocket);
    maxsock++;

    for (;;) { /* FOREVER - until a valid mon packet is received */
	FD_ZERO(&readset);
	FD_SET(mux->clientsocket, &readset);
	FD_SET(mux->monsocket, &readset);
	
	socksactive = select(maxsock, &readset, NULL, NULL, NULL);
	
	if (socksactive != -1) {
	    if (FD_ISSET(mux->clientsocket, &readset)) {
		spawn_client(mux->clientsocket);
	    }

	    if (FD_ISSET(mux->monsocket, &readset)) {
		if (recvmonpacket(mux, sourcelist, source, packet))
		    return;
	    }
	} else {
	    if (errno == EINTR)
		return;  /* signal received while waiting, bail out */
	}
    }
}
/* Receive a mon packet for mux. Checks if the source is allowed and returns the source found.
 * return 0 if no valid packet found 
 */
int
recvmonpacket(struct mux *mux, struct sourcelist *sourcelist, 
	      struct source **source, struct monpacket *packet) 

{
    struct sockaddr_in sind;
    u_int32_t sourceaddr;
    int size;
    unsigned int received;
    socklen_t sl;
    int tries;
    u_int32_t crc;

    received = 0;
    tries = 0;
    do {
	sl = sizeof(sind);

	size = recvfrom(mux->monsocket, (void *)packet + received, 
			sizeof(struct monpacket) - received, 
			0, (struct sockaddr *)&sind, &sl);
	if (size > 0)
	  received += size;

	tries++;
    } while ((size == -1) && 
	     (errno == EAGAIN || errno == EINTR) && 
	     (tries < MONMUX_MAXREADTRIES) &&
	     (received < sizeof(struct monpacket)));

    if ((size == -1) && 
	errno) 
      warning("recvfrom failed: %s", strerror(errno));

    sourceaddr = ntohl((u_int32_t)sind.sin_addr.s_addr);
    *source = find_source_ip(sourcelist, sourceaddr);
	    
    if (*source == NULL) {
	debug("ignored data from %u.%u.%u.%u",
	      IPAS4BYTES(sourceaddr));
	return 0;
    } else {
	/* check crc */
	crc = ntohl(packet->header.crc);
	packet->header.crc = 0;
	crc ^= crc32(packet, received);
	if ( crc != 0 ) {
	    warning("ignored packet with bad crc from %u.%u.%u.%u",
		    IPAS4BYTES(sourceaddr));
	    return 0;
	}
	/* check packet version */
	if (packet->header.mon_version != MON_PACKET_VER) {
	    warning("ignored packet with illegal type %d", 
		    packet->header.mon_version);
	    return 0;
	} else {
	    /* rewrite structs to host order */
	    packet->header.length = ntohs(packet->header.length);
	    packet->header.crc = ntohs(packet->header.crc);
	    packet->header.timestamp = ntohq(packet->header.timestamp);
		
	    if (flag_debug) 
		debug("good data received from %u.%u.%u.%u",
		      IPAS4BYTES(sourceaddr));
	    return 1;  /* good packet received */
	} 
    }
}
int 
acceptconnection(int sock)
{
    struct sockaddr_in sind;
    socklen_t len;
    u_int32_t clientaddr;
    int clientsock;

    if ((clientsock = accept(sock, (struct sockaddr *)&sind, &len)) < 0)
	fatal("Failed to accept an incoming connection. (.200%s)",
	      strerror(errno));

    clientaddr = ntohl((u_int32_t)sind.sin_addr.s_addr);
    info("Accepted incoming client connection from %u.%u.%u.%u",
	 IPAS4BYTES(clientaddr));

    return clientsock;
}   
