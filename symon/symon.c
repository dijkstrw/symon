/* $Id: symon.c,v 1.13 2002/03/31 14:27:47 dijkstra Exp $ */

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

#include "mon.h"
#include "net.h"
#include "monnet.h"
#include "readconf.h"
#include "data.h"
#include "error.h"

__BEGIN_DECLS
void alarmhandler(int);
__END_DECLS

struct muxlist muxlist = SLIST_HEAD_INITIALIZER(muxlist);

#ifdef MON_KVM
kvm_t *kvmd;
struct nlist mon_nl[] = {
    {"_ifnet"},      /* MON_IFNET = 0  (mon.h)*/
    {"_disklist"},   /* MON_DL    = 1  (mon.h)*/
    {""},
};

int 
kread(u_long addr, char *buf, int size)
{
    if (kvm_read(kvmd, addr, buf, size) != size)
	fatal("kvm_read:%s", kvm_geterr(kvmd));

    return (0);
}
#endif /* MON_KVM */

/* map stream types to inits and getters */
struct funcmap streamfunc[] = {
    {MT_IO,  init_io,  get_io},
    {MT_CPU, init_cpu, get_cpu},
    {MT_MEM, init_mem, get_mem},
    {MT_IF,  init_if,  get_if},
    {MT_EOT, NULL, NULL}
};

/* Alarmhandler that gets called every MON_INTERVAL */
void 
alarmhandler(int s) {
    /* EMPTY */
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
    struct itimerval alarminterval;
    struct stream *stream;
    struct mux *mux;
    char mon_buf[_POSIX2_LINE_MAX];
    int offset;

#ifdef MON_KVM
    char *nlistf = NULL, *memf = NULL;

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

    read_config_file("mon.conf");

#ifndef DEBUG
    if (daemon(0,0) != 0)
	fatal("daemonize failed");
#endif

    /* Init modules */
    inform("mon $Revision: 1.13 $ started");

    offset = 0;
    SLIST_FOREACH(mux, &muxlist, muxes) {
	connect2mux(mux);
	SLIST_FOREACH(stream, &mux->sl, streams) {
	    (streamfunc[stream->type].init)(stream->args);
	}
    }

    /* Setup signal handlers */
    signal(SIGALRM, alarmhandler);
    signal(SIGINT, exit); 
    signal(SIGQUIT, exit); 
    signal(SIGTERM, exit); 

    /* Setup alarm */
    timerclear(&alarminterval.it_interval);
    timerclear(&alarminterval.it_value);
    alarminterval.it_interval.tv_sec=
	alarminterval.it_value.tv_sec=MON_INTERVAL;

    if (setitimer(ITIMER_REAL, &alarminterval, NULL) != 0) {
	syslog(LOG_INFO,"alarm setup failed -- %s", strerror(errno));
	exit(1);
    }

    for (;;) {  /* forever */
	sleep(MON_INTERVAL*2);    /* alarm will always interrupt sleep */

	SLIST_FOREACH(mux, &muxlist, muxes) {
	    prepare_packet(mux);

	    SLIST_FOREACH(stream, &mux->sl, streams)
		stream_in_packet(stream, mux);

	    finish_packet(mux);

	    send_packet(mux);
	}
    }
    /* NOTREACHED */
}

















