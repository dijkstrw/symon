/* $Id: symux.c,v 1.8 2002/04/01 20:16:04 dijkstra Exp $ */

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
#include <netdb.h>

#include <machine/endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <rrd.h>

#include "data.h"
#include "error.h"
#include "limits.h"
#include "monmux.h"
#include "muxnet.h"
#include "net.h"
#include "readconf.h"
#include "xmalloc.h"

__BEGIN_DECLS
void exithandler();
void close_listensock();
void huphandler(int);
void signalhandler(int);
__END_DECLS

int flag_hup = 0;
int listen_sock;
fd_set fdset;
int maxfd;

void
exithandler(int s) {
    info("received signal - quitting");
    exit(1);
}
void 
close_listensock() 
{
    close(listen_sock);
}
void
huphandler(int s) {
    info("hup received");
    flag_hup = 1;
}
/* 
 * Monmux is the receiver of mon performance measurements.
 *
 * The main goals mon hopes to accomplish is:
 * - to take fine grained measurements of system parameters 
 * - with minimal performance impact 
 * - in a secure way.
 * 
 * Measuring system parameters (e.g. interfaces) sometimes means traversing
 * lists in kernel memory. Because of this the measurement of data has been
 * decoupled from the processing and storage of data. Storing the measured
 * information that mon provides is done by a second program, called monmux.
 * 
 * Mon can keep track of cpu, memory, disk and network interface
 * interactions. Mon was built specifically for OpenBSD.
 */
int 
main(int argc, char *argv[])
{
    struct monpacket packet;
    struct packedstream ps;
    char stringbuf[_POSIX2_LINE_MAX];
    struct muxlist mul, newmul;
    struct sourcelist sol, newsol;
    char *arg_ra[3];
    struct stream *stream;
    struct source *source;
    struct mux *mux;
    FILE *f;
    char *p;
    char *version;
    int ch;
    int offset;
    time_t t;
    
    SLIST_INIT(&mul);
    SLIST_INIT(&sol);
	
    /* prepare version number */
    version = xstrdup("$Revision: 1.8 $");
    version = strchr(version, ' ') + 1;
    p = strchr(version, '$');
    *--p = '\0';

    info("monmux version %s", version);

    /* reset flags */
    flag_debug = 0;
    flag_daemon = 0;

    while ((ch = getopt(argc, argv, "dv")) != -1) {
	switch (ch) {
	case 'd':
	    flag_debug = 1;
	    break;
	case 'v':
	    info("monmux version %s", version);
	default:
	    info("usage: %s [-d] [-v]", __progname);
	    exit(1);
	}
    }

    /* parse configuration file */
    if (!read_config_file(&mul, &sol, MONMUX_CONFIG_FILE))
	fatal("configuration contained errors; quitting");

    if (flag_debug != 1) {
	if (daemon(0,0) != 0)
	    fatal("daemonize failed");
	
	flag_daemon = 1;

	/* record pid */
	f = fopen(MONMUX_PID_FILE, "w");
	if (f) {
	    fprintf(f, "%u\n", (u_int) getpid());
	    fclose(f);
	}
    } else {
	info("program id=%d", (u_int) getpid());
    }

    mux = SLIST_FIRST(&mul);

    /* catch signals */
    signal(SIGHUP, huphandler);
    signal(SIGINT, exithandler); 
    signal(SIGQUIT, exithandler); 
    signal(SIGTERM, exithandler);
    signal(SIGTERM, exithandler); 

    listen_sock = getmuxsocket(mux);

    atexit(close_listensock);

    for (;;) {
	wait_for_packet(listen_sock, &sol, &source, &packet);

	if (flag_hup == 1) {
	    flag_hup = 0;

	    SLIST_INIT(&newmul);
	    SLIST_INIT(&newsol);

	    if (!read_config_file(&newmul, &newsol, MONMUX_CONFIG_FILE)) {
		info("new configuration contains errors; keeping old configuration");
		free_muxlist(&newmul);
		free_sourcelist(&newsol);
	    } else {
		free_muxlist(&mul);
		free_sourcelist(&sol);
		mul = newmul;
		sol = newsol;
		info("read configuration file succesfully");
	    }
	}

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
		    ps2strn(&ps, stringbuf + strlen(stringbuf), 
				sizeof(stringbuf) - strlen(stringbuf), 
				PS2STR_RRD);

		    arg_ra[2] = stringbuf;

		    rrd_update(3,arg_ra);

		    if (rrd_test_error()) {
			warning("rrd_update:%s",rrd_get_error());
			debug("%s %s %s", arg_ra[0], arg_ra[1], arg_ra[2]);
			rrd_clear_error();                                                            
		    }
		}
	    }
	}
    }
    /* NOT REACHED */
}
