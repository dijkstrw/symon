/* $Id: symux.c,v 1.13 2002/06/21 15:53:32 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2002 Willem Dijkstra
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 
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
#include "share.h"
#include "xmalloc.h"

__BEGIN_DECLS
void exithandler();
void huphandler(int);
void signalhandler(int);
__END_DECLS

int flag_hup = 0;
fd_set fdset;
int maxfd;

void
exithandler(int s) {
    info("received signal - quitting");
    exit(1);
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
    char *stringbuf;
    char *stringptr;
/*    char stringbuf[_POSIX2_LINE_MAX]; */
    struct muxlist mul, newmul;
    struct sourcelist sol, newsol;
    char *arg_ra[4];
    struct stream *stream;
    struct source *source;
    struct mux *mux;
    FILE *f;
    int ch;
    int offset;
    time_t t;
    int churnbuflen;

    SLIST_INIT(&mul);
    SLIST_INIT(&sol);
	
    /* reset flags */
    flag_debug = 0;
    flag_daemon = 0;

    while ((ch = getopt(argc, argv, "dv")) != -1) {
	switch (ch) {
	case 'd':
	    flag_debug = 1;
	    break;
	case 'v':
	    info("monmux version %s", MONMUX_VERSION);
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
    } 
    
    info("monmux version %s", MONMUX_VERSION);

    if (flag_debug == 1)
	info("program id=%d", (u_int) getpid());

    mux = SLIST_FIRST(&mul);

    churnbuflen = calculate_churnbuffer(&sol);
    debug("Size of churnbuffer = %d", churnbuflen);
    initshare(churnbuflen);

    /* catch signals */
    signal(SIGHUP, huphandler);
    signal(SIGINT, exithandler); 
    signal(SIGQUIT, exithandler); 
    signal(SIGTERM, exithandler);
    signal(SIGTERM, exithandler); 

    getmonsocket(mux);
    getclientsocket(mux);

    /* TODO    atexit(close_listensock); */

    for (;;) {
	waitfortraffic(mux, &sol, &source, &packet);

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

	/* put time:ip: into shared region */
	master_forbidread();
	t = (time_t) packet.header.timestamp;
	stringbuf = (char *)shared_getmem();
	sprintf(stringbuf, "%u:%u.%u.%u.%u:", t,
		IPAS4BYTES(source->ip));
	stringptr = stringbuf + strlen(stringbuf);

	while (offset < packet.header.length) {
	    offset += sunpack(packet.data + offset, &ps);

	    /* find stream in source */
	    stream = find_source_stream(source, ps.type, ps.args);

	    if (stream != NULL) {
		/* put type and args in for clients */
		sprintf(stringptr, "%s:", type2str(ps.type));
		stringptr += strlen(stringptr);
		sprintf(stringptr, "%s:", ((ps.args == NULL)?"0":ps.args));
		stringptr += strlen(stringptr);
		ps2strn(&ps, stringptr,
			(shared_getmaxlen() - (stringptr - stringbuf)),
			PS2STR_RRD);

		if (stream->file != NULL) {
		    /* save if file specified */
		    arg_ra[0] = "rrdupdate";
		    arg_ra[1] = "--";
		    arg_ra[2] = stream->file;
		    
		    arg_ra[3] = stringptr;
		    rrd_update(4, arg_ra);

		    if (rrd_test_error()) {
			warning("rrd_update:%s",rrd_get_error());
			warning("%s %s %s %s", arg_ra[0], arg_ra[1], arg_ra[2], arg_ra[3]);
			rrd_clear_error();                                                            
		    } else {
			if (flag_debug == 1) 
			    debug("%s %s %s %s", arg_ra[0], arg_ra[1], arg_ra[2], arg_ra[3]);
		    }
		}
		stringptr += strlen(stringptr);
		sprintf(stringptr, ";");
		stringptr += strlen(stringptr);
	    }
	}
	/* packet = parsed and in ascii in shared region -> copy to clients */
	shared_setlen((stringptr - stringbuf));
	master_permitread();
    }
    /* NOT REACHED */
}
