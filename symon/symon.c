/* $Id: symon.c,v 1.24 2002/09/14 15:49:39 dijkstra Exp $ */

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

#include <sys/param.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "data.h"
#include "error.h"
#include "symon.h"
#include "symonnet.h"
#include "net.h"
#include "readconf.h"
#include "xmalloc.h"

__BEGIN_DECLS
void alarmhandler(int);
void exithandler(int);
void huphandler(int);
__END_DECLS

int flag_hup = 0;

/* map stream types to inits and getters */
struct funcmap streamfunc[] = {
    {MT_IO,  init_io,  gets_io, get_io},  /* gets_io obtains entire io state, get_io = just a copy */
    {MT_CPU, init_cpu, NULL,    get_cpu},
    {MT_MEM, init_mem, NULL,    get_mem},
    {MT_IF,  init_if,  NULL,    get_if},
    {MT_PF,  init_pf,  NULL,    get_pf},
    {MT_EOT, NULL, NULL}
};

/* Alarmhandler that gets called every SYMON_INTERVAL */
void 
alarmhandler(int s) {
    /* EMPTY */
}
void
exithandler(int s) {
    info("received signal %d - quitting", s);
    exit(1);
}
void
huphandler(int s) {
    info("hup received");
    flag_hup = 1;
}
/* 
 * Symon is a system measurement utility. 
 *
 * The main goals symon hopes to accomplish is:
 * - to take fine grained measurements of system parameters 
 * - with minimal performance impact 
 * - in a secure way.
 * 
 * Measuring system parameters (e.g. interfaces) sometimes means traversing
 * lists in kernel memory. Because of this the measurement of data has been
 * decoupled from the processing and storage of data. Storing the measured
 * information that symon provides is done by a second program, called symux.
 * 
 * Symon can keep track of cpu, memory, disk and network interface
 * interactions. Symon was built specifically for OpenBSD.
 */
int 
main(int argc, char *argv[])
{
    struct muxlist mul, newmul;
    struct itimerval alarminterval;
    struct stream *stream;
    struct mux *mux;
    FILE *f;
    int ch;
    int i;

    SLIST_INIT(&mul);

    /* reset flags */
    flag_debug = 0;
    flag_daemon = 0;

    while ((ch = getopt(argc, argv, "dv")) != -1) {
	switch (ch) {
	case 'd':
	    flag_debug = 1;
	    break;
	case 'v':
	    info("symon version %s", SYMON_VERSION);
	default:
	    info("usage: %s [-d] [-v]", __progname);
	    exit(1);
	}
    }

    setegid(getgid());
    setgid(getgid());

    if (flag_debug != 1) {
	if (daemon(0,0) != 0)
	    fatal("daemonize failed");
	
	flag_daemon = 1;

	/* record pid */
	f = fopen(SYMON_PID_FILE, "w");
	if (f) {
	    fprintf(f, "%u\n", (u_int) getpid());
	    fclose(f);
	}
    } 

    info("symon version %s", SYMON_VERSION);

    if (!read_config_file(&mul, SYMON_CONFIG_FILE))
	fatal("configuration contained errors; quitting");

    if (flag_debug == 1)
	info("program id=%d", (u_int) getpid());

    /* Setup signal handlers */
    signal(SIGALRM, alarmhandler);
    signal(SIGHUP, huphandler);
    signal(SIGINT, exithandler); 
    signal(SIGQUIT, exithandler); 
    signal(SIGTERM, exithandler); 

    /* Prepare crc32 */
    init_crc32();

    /* init modules */
    SLIST_FOREACH(mux, &mul, muxes) {
	connect2mux(mux);
	SLIST_FOREACH(stream, &mux->sl, streams) {
	    (streamfunc[stream->type].init)(stream->args);
	}
    }

    /* Setup alarm */
    timerclear(&alarminterval.it_interval);
    timerclear(&alarminterval.it_value);
    alarminterval.it_interval.tv_sec=
	alarminterval.it_value.tv_sec=SYMON_INTERVAL;

    if (setitimer(ITIMER_REAL, &alarminterval, NULL) != 0) {
	fatal("alarm setup failed -- %s", strerror(errno));
    }

    for (;;) {  /* FOREVER */
	sleep(SYMON_INTERVAL*2);    /* alarm will always interrupt sleep */

	if (flag_hup == 1) {
	    flag_hup = 0;

	    SLIST_INIT(&newmul);

	    if (!read_config_file(&newmul, SYMON_CONFIG_FILE)) {
		info("new configuration contains errors; keeping old configuration");
		free_muxlist(&newmul);
	    } else {
		free_muxlist(&mul);
		mul = newmul;
		info("read configuration file succesfully");

		/* init modules */
		SLIST_FOREACH(mux, &mul, muxes) {
		    connect2mux(mux);
		    SLIST_FOREACH(stream, &mux->sl, streams) {
			(streamfunc[stream->type].init)(stream->args);
		    }
		}
	    }
	}

	/* populate data space for modules that get all their measurements in
           one go */
	for (i=0; i<MT_EOT; i++)
	    if (streamfunc[i].gets != NULL)
		(streamfunc[i].gets)();

	SLIST_FOREACH(mux, &mul, muxes) {
	    prepare_packet(mux);
	    
	    SLIST_FOREACH(stream, &mux->sl, streams)
		stream_in_packet(stream, mux);

	    finish_packet(mux);

	    send_packet(mux);
	}
    }
    /* NOTREACHED */
}
