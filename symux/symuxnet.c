/*
 * $Id: symuxnet.c,v 1.1 2002/03/29 15:18:05 dijkstra Exp $
 *
 * Holds all network functions for mon
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

#include "data.h"
#include "error.h"
#include "muxnet.h"
#include "net.h"

/* connect to a mux */
int getmuxsocket(struct mux *mux) {
    struct sockaddr_in sockaddr;
    int sock;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	fatal("Could not obtain socket: %.200s", strerror(errno));
    }

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(mux->port);
    sockaddr.sin_addr.s_addr = htonl(mux->ip);
    bzero(&sockaddr.sin_zero, 8);

    if (bind(sock, (struct sockaddr *) &sockaddr, sizeof(struct sockaddr)) == -1) {
	fatal("Could not bind socket: %.200s", strerror(errno));
    }

    return sock;
}

/* wait for a packet on listensock from a source in sourcelist
 * returns the source and packet
 */
void wait_for_packet(listen_sock, sourcelist, source, packet) 
    int listen_sock;
    struct sourcelist *sourcelist;
    struct source **source;
    struct monpacket *packet;
{
    struct sockaddr_in sind;
    u_int32_t sourceaddr;
    int size;
    socklen_t sl;

    for (;;) {
	sl = sizeof(sind);
	size = recvfrom(listen_sock, (void *)packet, sizeof(struct monpacket), 0, (struct sockaddr *)&sind, &sl);
	if ( size < 0 ) {
	    if (errno != EAGAIN) {
		fatal("recvfrom failed: %s", strerror(errno));
	    }
	} else {
	    sourceaddr = ntohl((u_int32_t)sind.sin_addr.s_addr);
	    *source = find_source_ip(sourcelist, sourceaddr);
	    
	    if (*source == NULL) {
		syslog(LOG_INFO, "ignored data from %u.%u.%u.%u",
		       (sourceaddr >> 24), (sourceaddr >> 16) & 0xff, 
		       (sourceaddr >> 8) & 0xff, sourceaddr & 0xff);
	    } else { 
		/* check packet version */
		if (packet->header.mon_version != MON_STREAMVER) {
		    syslog(LOG_INFO, "ignored packet with illegal type %d", 
			   packet->header.mon_version);
		} else {
		    /* rewrite structs to host order */
		    packet->header.length = ntohs(packet->header.length);
		    packet->header.crc = ntohs(packet->header.crc);
		    packet->header.timestamp = ntohq(packet->header.timestamp);
		
		    /* check crc */
		    if (!check_crc_packet(packet)) {
			syslog(LOG_INFO, "crc failure for packet from %u.%u.%u.%u",
			       (lookup_ip >> 24), (lookup_ip >> 16) & 0xff, 
			       (lookup_ip >> 8) & 0xff, lookup_ip & 0xff);
		    } else
			return;  /* good packet received */
		      
		} 
	    }
	}
    }
}

int check_crc_packet(packet)
    struct monpacket *packet;
{
    // TODO: sane and efficient crc check
    return 1;
}
