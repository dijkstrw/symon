/*
 * $Id: symux.c,v 1.5 2002/03/29 15:17:55 dijkstra Exp $
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
    struct mux *mux;
    struct monpacket packet;
    struct packedstream ps;
    struct source *source;

    char stringbuf[_POSIX2_LINE_MAX];
    int offset;

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
	time_t t;
	
	wait_for_packet(listen_sock, &sourcelist, &source, &packet);

	/* print time in packet */
	t = (time_t) packet.header.timestamp;
	ctime_r(&t, stringbuf);
	printf("Packet received with ts=%s", stringbuf);

	offset = 0;
	while (offset < packet.header.length) {
	    offset += sunpack(packet.data + offset, &ps);
	    psdata2strn(&ps, stringbuf, sizeof(stringbuf));
	    printf("type %d(%s):%s\n", ps.type, ps.args, stringbuf);
	}
    }
    /* NOT REACHED */
}




