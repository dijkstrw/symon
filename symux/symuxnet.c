/* $Id: symuxnet.c,v 1.2 2002/03/31 14:27:50 dijkstra Exp $ */

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

#include "data.h"
#include "error.h"
#include "muxnet.h"
#include "net.h"

/* Obtain a mux socket for listening */
int 
getmuxsocket(struct mux *mux) 
{
    struct sockaddr_in sockaddr;
    int sock;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	fatal("Could not obtain socket: %.200s", strerror(errno));

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(mux->port);
    sockaddr.sin_addr.s_addr = htonl(mux->ip);
    bzero(&sockaddr.sin_zero, 8);

    if (bind(sock, (struct sockaddr *) &sockaddr, sizeof(struct sockaddr)) == -1)
	fatal("Could not bind socket: %.200s", strerror(errno));

    return sock;
}
/*
 * Wait for a packet on <listen_sock> from a source in <sourcelist>
 * Returns the <source> and <packet>
 */
void
wait_for_packet(int listen_sock, struct sourcelist *sourcelist, 
		struct source **source, struct monpacket *packet) 
{
    struct sockaddr_in sind;
    u_int32_t sourceaddr;
    int size;
    socklen_t sl;

    for (;;) { /* FOREVER */
	sl = sizeof(sind);
	size = recvfrom(listen_sock, (void *)packet, sizeof(struct monpacket), 
			0, (struct sockaddr *)&sind, &sl);

	if ( size < 0 ) {
	    if (errno != EAGAIN) {
		fatal("recvfrom failed: %s", strerror(errno));
	    }
	} else {
	    sourceaddr = ntohl((u_int32_t)sind.sin_addr.s_addr);
	    *source = find_source_ip(sourcelist, sourceaddr);
	    
	    if (*source == NULL) {
		warning("ignored data from %u.%u.%u.%u",
		       (sourceaddr >> 24), (sourceaddr >> 16) & 0xff, 
		       (sourceaddr >> 8) & 0xff, sourceaddr & 0xff);
	    } else { 
		/* check packet version */
		if (packet->header.mon_version != MON_PACKET_VER) {
		    warning("ignored packet with illegal type %d", 
			   packet->header.mon_version);
		} else {
		    /* rewrite structs to host order */
		    packet->header.length = ntohs(packet->header.length);
		    packet->header.crc = ntohs(packet->header.crc);
		    packet->header.timestamp = ntohq(packet->header.timestamp);
		
		    /* check crc */
		    if (!check_crc_packet(packet)) {
			warning("crc failure for packet from %u.%u.%u.%u",
			       (lookup_ip >> 24), (lookup_ip >> 16) & 0xff, 
			       (lookup_ip >> 8) & 0xff, lookup_ip & 0xff);
		    } else
			return;  /* good packet received */
		      
		} 
	    }
	}
    }
}
int 
check_crc_packet(struct monpacket *packet)
{
    /* TODO: sane and efficient crc check */
    return 1;
}
