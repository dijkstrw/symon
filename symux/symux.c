/* $Id: symux.c,v 1.7 2002/03/31 14:27:50 dijkstra Exp $ */

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

__BEGIN_DECLS
void signalhandler(int);
void close_listensock();
__END_DECLS

struct muxlist muxlist = SLIST_HEAD_INITIALIZER(muxlist);
struct sourcelist sourcelist = SLIST_HEAD_INITIALIZER(sourcelist);

int listen_sock;
fd_set fdset;
int maxfd;
int signal_seen;

void 
signalhandler(int s) 
{
    signal_seen = s;
}

void 
close_listensock() 
{
    close(listen_sock);
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
    read_config_file(config_file);

#ifndef DEBUG
    if (daemon(0,0) != 0)
	fatal("daemonize failed");
#endif

    inform("monmux $Revision: 1.7 $ started");

    mux = SLIST_FIRST(&muxlist);

    /* catch signals */
    signal(SIGTERM, signalhandler);

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
		    ps2strn(&ps, stringbuf + strlen(stringbuf), 
				sizeof(stringbuf) - strlen(stringbuf), 
				PS2STR_RRD);

		    arg_ra[2] = stringbuf;

#ifdef DEBUG
		    inform("%d(%s)='%s'",ps.type,ps.args,stringbuf);
#else
		    rrd_update(3,arg_ra);

		    if (rrd_test_error()) {
			warning("rrd_update:%s",rrd_get_error());
			rrd_clear_error();                                                            
		    }
#endif
		}
	    }
	}
    }
    /* NOT REACHED */
}
