/*
 * $Id: symux.c,v 1.4 2002/03/22 16:40:22 dijkstra Exp $
 *
 * Daemon that multiplexes incoming mon traffic to subscribed clients
 * and archives it periodically in rrd files.
 */
#include <sys/types.h>
#include <machine/endian.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>

#include "error.h"
#include "monmux.h"
#include "readconf.h"
#include "limits.h"
#include "data.h"
#include "muxnet.h"
#include "net.h"

struct muxlist muxlist = SLIST_HEAD_INITIALIZER(muxlist);
struct sourcelist sourcelist = SLIST_HEAD_INITIALIZER(sourcelist);

int listen_sock;
fd_set fdset;
int maxfd;
int signal_seen;

void signalhandler(s) 
    int s;
{
    signal_seen = s;
}

void close_listensock() 
{
    close(listen_sock);
}

int main(argc, argv)
    int argc;
    char *argv[];
{
    struct sockaddr_in sind;
    struct mux *mux;
    struct packedstream ps;
    struct source *source;

    char netbuf[_POSIX2_LINE_MAX];
    char stringbuf[_POSIX2_LINE_MAX];
    int offset, size;
    socklen_t sl;

    /* parse configuration file */
    read_config_file("monmux.conf");

    mux = SLIST_FIRST(&muxlist);

    /* catch signals */
//    signal(SIGALRM, alarmhandler);
    signal(SIGINT, signalhandler);
    signal(SIGQUIT, signalhandler);
    signal(SIGTERM, signalhandler);
    signal(SIGXCPU, signalhandler);
    signal(SIGXFSZ, signalhandler);
    signal(SIGUSR1, signalhandler);
    signal(SIGUSR2, signalhandler);

    listen_sock = getmuxsocket(mux);

    atexit(close_listensock);

    for (;;) {
	sl = sizeof(sind);
	size = recvfrom(listen_sock, netbuf, sizeof(netbuf), 0, (struct sockaddr *)&sind, &sl);
	if ( size < 0 ) {
	    /* do sth */
	} else {
	    time_t t;
	    source = find_source_ip(&sourcelist, ntohl((u_int32_t)sind.sin_addr.s_addr));

	    if (source == NULL) {
		syslog(LOG_INFO, "ignored data from %u.%u.%u.%u",
		       (lookup_ip >> 24), (lookup_ip >> 16) & 0xff, 
		       (lookup_ip >> 8) & 0xff, lookup_ip & 0xff);
	    } /* else { */
	    
		/* TODO: check mon type and get length */
		offset = getpreamble(netbuf, &t);
		while ((size - offset) > 0) {
		    offset += sunpack(netbuf + offset, &ps);
		    psdata2strn(&ps, stringbuf, sizeof(stringbuf));
		    printf("type %d(%s):%s\n", ps.type, ps.args, stringbuf);
//		}
	    }
//	    process(ntohl(sind.sin_addr.s_addr), cbuf);
	}
    }
    /* NOT REACHED */
}




