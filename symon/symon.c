/* $Id: symon.c,v 1.22 2002/08/31 16:09:55 dijkstra Exp $ */

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
#include "mon.h"
#include "monnet.h"
#include "net.h"
#include "readconf.h"
#include "xmalloc.h"

__BEGIN_DECLS
void alarmhandler(int);
void exithandler(int);
void huphandler(int);
__END_DECLS

int flag_hup = 0;

#ifdef MON_KVM
kvm_t *kvmd;
struct nlist mon_nl[] = {
    {"_disklist"},   /* MON_DL    = 0  (mon.h)*/
    {""},
};
/* Read kernel memory */
int 
kread(u_long addr, char *buf, int size)
{
    if (kvm_read(kvmd, addr, buf, size) != size) {
	warning("kvm_read:%s", kvm_geterr(kvmd));
	return 1;
    }

    return 0;
}
#else /* MON_KVM */
int 
kread(u_long addr, char *buf, int size)
{
    warning("kvm_read not compiled in, calling probe will signal failure");

    return 0;
}
#endif
/* map stream types to inits and getters */
struct funcmap streamfunc[] = {
    {MT_IO,  init_io,  get_io},
    {MT_CPU, init_cpu, get_cpu},
    {MT_MEM, init_mem, get_mem},
    {MT_IF,  init_if,  get_if},
    {MT_PF,  init_pf,  get_pf},
    {MT_EOT, NULL, NULL}
};

/* Alarmhandler that gets called every MON_INTERVAL */
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
 * Mon is a system measurement utility. 
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
    struct muxlist mul, newmul;
    struct itimerval alarminterval;
    struct stream *stream;
    struct mux *mux;
    FILE *f;
#ifdef MON_KVM
    char mon_buf[_POSIX2_LINE_MAX];
    char *nlistf = NULL, *memf = NULL;
#endif
    int ch;

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
	    info("mon version %s", MON_VERSION);
	default:
	    info("usage: %s [-d] [-v]", __progname);
	    exit(1);
	}
    }

#ifdef MON_KVM
    /* Populate our kernel name list */
    if ((kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, mon_buf)) == NULL)
	fatal("kvm_open: %s", mon_buf);
    
    if (kvm_nlist(kvmd, mon_nl) < 0 || mon_nl[0].n_type == 0) {
	if (nlistf)
	    fatal("%s: no namelist", nlistf);
	else
	    fatal("no namelist");
    }
#endif /* MON_KVM */

    setegid(getgid());
    setgid(getgid());

    if (flag_debug != 1) {
	if (daemon(0,0) != 0)
	    fatal("daemonize failed");
	
	flag_daemon = 1;

	/* record pid */
	f = fopen(MON_PID_FILE, "w");
	if (f) {
	    fprintf(f, "%u\n", (u_int) getpid());
	    fclose(f);
	}
    } 

    info("mon version %s", MON_VERSION);

    if (!read_config_file(&mul, MON_CONFIG_FILE))
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
	alarminterval.it_value.tv_sec=MON_INTERVAL;

    if (setitimer(ITIMER_REAL, &alarminterval, NULL) != 0) {
	fatal("alarm setup failed -- %s", strerror(errno));
    }

    for (;;) {  /* FOREVER */
	sleep(MON_INTERVAL*2);    /* alarm will always interrupt sleep */

	if (flag_hup == 1) {
	    flag_hup = 0;

	    SLIST_INIT(&newmul);

	    if (!read_config_file(&newmul, MON_CONFIG_FILE)) {
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
