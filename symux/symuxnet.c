/* $Id: symuxnet.c,v 1.13 2003/12/20 16:30:44 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2003 Willem Dijkstra
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

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include "data.h"
#include "error.h"
#include "symux.h"
#include "symuxnet.h"
#include "net.h"
#include "xmalloc.h"
#include "share.h"

__BEGIN_DECLS
int check_crc_packet(struct symonpacket *);
__END_DECLS

/* Obtain sockets for incoming symon traffic */
int
get_symon_sockets(struct mux * mux)
{
    struct source *source;
    struct sockaddr_storage sockaddr;
    int family, nsocks;
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    nsocks = 0;

    /* generate the udp listen socket specified in the mux statement */
    get_mux_sockaddr(mux, SOCK_DGRAM);

    /* iterate over our sources to determine what types of sockets we need */
    SLIST_FOREACH(source, &mux->sol, sources) {
	get_source_sockaddr(source);

	family = source->sockaddr.ss_family;
	/* do we have a socket for this type of family */
	if (mux->symonsocket[family] <= 0) {
	    if ((mux->symonsocket[family] = socket(family, SOCK_DGRAM, 0)) != -1) {
		/*
		 * does the mux statement specify a specific destination
		 * address
		 */
		if (mux->sockaddr.ss_family == family) {
		    cpysock((struct sockaddr *) & mux->sockaddr, &sockaddr);
		} else {
		    get_inaddrany_sockaddr(&sockaddr, family, SOCK_DGRAM, mux->port);
		}

		if (bind(mux->symonsocket[family], (struct sockaddr *) & sockaddr,
			 sockaddr.ss_len) == -1)
		    close(mux->symonsocket[family]);
		else {
		    if (getnameinfo((struct sockaddr *) & sockaddr, sockaddr.ss_len,
				    hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
				    NI_NUMERICHOST | NI_NUMERICSERV)) {
			info("getnameinfo error - cannot determine numeric hostname and service");
			info("listening for incoming symon traffic for family %d", family);
		    } else
			info("listening for incoming symon traffic on udp %.200s %.200s", hbuf, sbuf);

		    nsocks++;
		}
	    }
	}
    }
    return nsocks;
}
/* Obtain a listen socket for new clients */
int
get_client_socket(struct mux * mux)
{
    struct addrinfo hints, *res;
    int error, sock;

    if ((sock = socket(mux->sockaddr.ss_family, SOCK_STREAM, 0)) == -1)
	fatal("could not obtain socket: %.200s", strerror(errno));

    bzero((void *) &hints, sizeof(struct addrinfo));

    hints.ai_family = mux->sockaddr.ss_family;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;
    hints.ai_socktype = SOCK_STREAM;

    if ((error = getaddrinfo(mux->addr, mux->port, &hints, &res)) != 0)
	fatal("could not get address information for %.200s:%.200s - %.200s",
	      mux->addr, mux->port, gai_strerror(error));

    if (bind(sock, (struct sockaddr *) res->ai_addr, res->ai_addrlen) == -1)
	fatal("could not bind socket: %.200s", strerror(errno));

    freeaddrinfo(res);

    if (listen(sock, SYMUX_TCPBACKLOG) == -1)
	fatal("could not listen to socket: %.200s", strerror(errno));

    fcntl(sock, O_NONBLOCK);
    mux->clientsocket = sock;

    info("listening for incoming connections on tcp %.200s %.200s",
	 mux->addr, mux->port);

    return sock;
}
/*
 * Wait for traffic (symon reports from a source in sourclist | clients trying to connect
 * Returns the <source> and <packet>
 * Silently forks off clienthandlers
 */
void
wait_for_traffic(struct mux * mux, struct source ** source,
		 struct symonpacket * packet)
{
    fd_set readset;
    int i;
    int socksactive;
    int maxsock;

    for (;;) {			/* FOREVER - until a valid symon packet is
				 * received */
	FD_ZERO(&readset);
	FD_SET(mux->clientsocket, &readset);

	maxsock = mux->clientsocket;

	for (i = 0; i < AF_MAX; i++) {
	    if (mux->symonsocket[i] > 0) {
		FD_SET(mux->symonsocket[i], &readset);
		maxsock = ((maxsock < mux->symonsocket[i]) ? mux->symonsocket[i] :
			   maxsock);
	    }
	}

	maxsock++;
	socksactive = select(maxsock, &readset, NULL, NULL, NULL);

	if (socksactive != -1) {
	    if (FD_ISSET(mux->clientsocket, &readset)) {
		spawn_client(mux->clientsocket);
	    }

	    for (i = 0; i < AF_MAX; i++)
		if (FD_ISSET(mux->symonsocket[i], &readset)) {
		    if (recv_symon_packet(mux, i, source, packet))
			return;
		}
	} else {
	    if (errno == EINTR)
		return;		/* signal received while waiting, bail out */
	}
    }
}
/* Receive a symon packet for mux. Checks if the source is allowed and returns the source found.
 * return 0 if no valid packet found
 */
int
recv_symon_packet(struct mux * mux, int socknr, struct source ** source,
		  struct symonpacket * packet)
{
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    struct sockaddr_storage sind;
    socklen_t sl;
    int size, tries;
    unsigned int received;
    u_int32_t crc;

    received = 0;
    tries = 0;
    bzero((void *) hbuf, sizeof(hbuf));
    bzero((void *) sbuf, sizeof(sbuf));

    do {
	sl = sizeof(sind);

	size = recvfrom(mux->symonsocket[socknr], (void *) packet->data + received,
			sizeof(struct symonpacket) - received,
			0, (struct sockaddr *) & sind, &sl);
	if (size > 0)
	    received += size;

	tries++;
    } while ((size == -1) &&
	     (errno == EAGAIN || errno == EINTR) &&
	     (tries < SYMUX_MAXREADTRIES) &&
	     (received < sizeof(packet->data)));

    if ((size == -1) &&
	errno)
	warning("recvfrom failed: %.200s", strerror(errno));

    *source = find_source_sockaddr(&mux->sol, (struct sockaddr *) & sind);

    if (*source == NULL) {
	getnameinfo((struct sockaddr *) & sind, sind.ss_len, hbuf, sizeof(hbuf),
		    sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
	debug("ignored data from %.200s:%.200s", hbuf, sbuf);
	return 0;
    } else {
	/* get header stream */
	mux->offset = getheader(packet->data, &packet->header);
	/* check crc */
	crc = packet->header.crc;
	packet->header.crc = 0;
	setheader(packet->data, &packet->header);
	crc ^= crc32(packet->data, received);
	if (crc != 0) {
	    getnameinfo((struct sockaddr *) & sind, sind.ss_len, hbuf, sizeof(hbuf),
			sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
	    warning("ignored packet with bad crc from %.200s:%.200s",
		    hbuf, sbuf);
	    return 0;
	}
	/* check packet version */
	if (packet->header.symon_version != SYMON_PACKET_VER) {
	    getnameinfo((struct sockaddr *) & sind, sind.ss_len, hbuf, sizeof(hbuf),
			sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
	    warning("ignored packet with wrong version %d from %.200s:%.200s",
		    packet->header.symon_version, hbuf, sbuf);
	    return 0;
	} else {
	    if (flag_debug) {
		getnameinfo((struct sockaddr *) & sind, sind.ss_len, hbuf, sizeof(hbuf),
		       sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
		debug("good data received from %.200s:%.200s", hbuf, sbuf);
	    }
	    return 1;		/* good packet received */
	}
    }
}
int
accept_connection(int sock, char *peername, int peernamesize)
{
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    struct sockaddr_storage sind;
    socklen_t len;
    int clientsock;

    bzero((void *) hbuf, sizeof(hbuf));
    bzero((void *) sbuf, sizeof(sbuf));

    if ((clientsock = accept(sock, (struct sockaddr *) & sind, &len)) < 0)
	fatal("failed to accept an incoming connection. (%.200s)",
	      strerror(errno));


    if (getnameinfo((struct sockaddr *) & sind, sind.ss_len, hbuf, sizeof(hbuf),
		    sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV))
	snprintf(peername, peernamesize, "<unknown>");
    else
	snprintf(peername, peernamesize, "%.200s:%.200s", hbuf, sbuf);

    return clientsock;
}
