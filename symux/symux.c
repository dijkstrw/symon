/*
 * $Id: symux.c,v 1.6 2002/03/29 16:28:15 dijkstra Exp $
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
#include <stdio.h>
#include <rrd.h>

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
    struct stream *stream;

    char stringbuf[_POSIX2_LINE_MAX];
    int offset;
    char *arg_ra[3];
    time_t t;
	
    /* parse configuration file */
    read_config_file("monmux.conf");

#ifndef DEBUG
    if (daemon(0,0) != 0) {
	syslog(LOG_ALERT, "daemonize failed -- exiting");
	fatal("daemonize failed");
    }
#endif

    syslog(LOG_INFO, "monmux $Revision: 1.6 $ started");

    mux = SLIST_FIRST(&muxlist);

    /* catch signals */
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
	wait_for_packet(listen_sock, &sourcelist, &source, &packet);

	offset = 0;
	while (offset < packet.header.length) {
	    offset += sunpack(packet.data + offset, &ps);

	    /* find stream in source */
	    stream = find_source_stream(source, ps.type, ps.args);
	    if (stream != NULL) {
		if (stream->file != NULL) {
		    /* save if file specified */
		    arg_ra[0] = "rrd_update";
		    arg_ra[1] = stream->file;

		    t = (time_t) packet.header.timestamp;
		    sprintf(stringbuf, "%u", t);
		    psdata2strn(&ps, stringbuf + strlen(stringbuf), 
				sizeof(stringbuf) - strlen(stringbuf), 
				PS2STR_RRD);

		    arg_ra[2] = stringbuf;

#ifdef DEBUG
		    syslog(LOG_INFO,"%d(%s)='%s'",ps.type,ps.args,stringbuf);
#else
		    rrd_update(3,arg_ra);

		    if (rrd_test_error()) {
			syslog(LOG_INFO,"rrd_update:%s",rrd_get_error());
			rrd_clear_error();                                                            
		    }
#endif
		}
	    }
	}
    }
    /* NOT REACHED */
}




