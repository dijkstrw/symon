/* $Id: symon.c,v 1.33 2004/02/24 22:13:20 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2003 Willem Dijkstra
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
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
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
void drop_priviledges();
void exithandler(int);
void huphandler(int);
void set_stream_use(struct muxlist *);
__END_DECLS

int flag_unsecure = 0;
int flag_hup = 0;
int symon_interval = SYMON_DEFAULT_INTERVAL;

/* map stream types to inits and getters */
struct funcmap streamfunc[] = {
    {MT_IO1, 0, NULL, init_io, gets_io, get_io},
    {MT_CPU, 0, NULL, init_cpu, NULL, get_cpu},
    {MT_MEM, 0, NULL, init_mem, NULL, get_mem},
    {MT_IF, 0, NULL, init_if, NULL, get_if},
    {MT_PF, 0, privinit_pf, init_pf, NULL, get_pf},
    {MT_DEBUG, 0, NULL, init_debug, NULL, get_debug},
    {MT_PROC, 0, NULL, init_proc, gets_proc, get_proc},
    {MT_MBUF, 0, NULL, init_mbuf, NULL, get_mbuf},
    {MT_SENSOR, 0, NULL, init_sensor, NULL, get_sensor},
    {MT_IO2, 0, NULL, init_io, gets_io, get_io},
    {MT_EOT, 0, NULL, NULL, NULL}
};

void
set_stream_use(struct muxlist *mul)
{
    struct mux *mux;
    struct stream *stream;
    int i;

    for (i = 0; i < MT_EOT; i++)
	streamfunc[i].used = 0;


    SLIST_FOREACH(mux, mul, muxes) {
	SLIST_FOREACH(stream, &mux->sl, streams)
	    streamfunc[stream->type].used = 1;
    }
}
void
drop_priviledges(int unsecure)
{
    struct passwd *pw;

    if (unsecure) {
	if (setegid(getgid()) || setgid(getgid()) ||
	    seteuid(getuid()) || setuid(getuid()))
	    fatal("can't drop priviledges: %.200s", strerror(errno));
    } else {
	if ((pw = getpwnam(SYMON_USER)) == NULL)
	    fatal("could not get user information for user '%.200s': %.200s",
		  SYMON_USER, strerror(errno));

	if (chroot(pw->pw_dir) < 0)
	    fatal("chroot failed: %.200s", strerror(errno));

	if (chdir("/") < 0)
	    fatal("chdir / failed: %.200s", strerror(errno));

	if (setgroups(1, &pw->pw_gid) ||
	    setegid(pw->pw_gid) || setgid(pw->pw_gid) ||
	    seteuid(pw->pw_uid) || setuid(pw->pw_uid))
	    fatal("can't drop privileges: %.200s", strerror(errno));
    }
}
/* alarmhandler that gets called every symon_interval */
void
alarmhandler(int s)
{
    /* EMPTY */
}
void
exithandler(int s)
{
    info("received signal %d - quitting", s);
    exit(1);
}
void
huphandler(int s)
{
    info("hup received");
    flag_hup = 1;
}
/*
 * Symon is a system measurement utility.
 *
 * The main goals symon hopes to accomplish are:
 * - to take fine grained measurements of system parameters
 * - with minimal performance impact
 * - in a secure way.
 *
 * Measurements are processed by a second program called symux. symon and symux
 * communicate via udp.
 */
int
main(int argc, char *argv[])
{
    struct muxlist mul, newmul;
    struct itimerval alarminterval;
    struct stream *stream;
    struct mux *mux;
    FILE *pidfile;
    char *cfgpath;
    int ch;
    int i;

    SLIST_INIT(&mul);

    /* reset flags */
    flag_debug = 0;
    flag_daemon = 0;
    flag_unsecure = 0;

    cfgpath = SYMON_CONFIG_FILE;

    while ((ch = getopt(argc, argv, "dvuf:")) != -1) {
	switch (ch) {
	case 'd':
	    flag_debug = 1;
	    break;

	case 'f':
	    cfgpath = xstrdup(optarg);
	    break;

	case 'u':
	    flag_unsecure = 1;
	    break;

	case 'v':
	    info("symon version %s", SYMON_VERSION);
	default:
	    info("usage: %s [-d] [-u] [-v] [-f cfgfile]", __progname);
	    exit(EX_USAGE);
	}
    }

    if (!read_config_file(&mul, cfgpath))
	fatal("configuration file contained errors - aborting");

    set_stream_use(&mul);

    /* open resources that might not be available after priviledge drop */
    for (i = 0; i < MT_EOT; i++)
	if (streamfunc[i].used && (streamfunc[i].privinit != NULL))
	    (streamfunc[i].privinit) ();

    if ((pidfile = fopen(SYMON_PID_FILE, "w")) == NULL)
	warning("could not open \"%.200s\", %.200s", SYMON_PID_FILE,
		strerror(errno));

    drop_priviledges(flag_unsecure);

    if (flag_debug != 1) {
	if (daemon(0, 0) != 0)
	    fatal("daemonize failed: %.200s", strerror(errno));

	flag_daemon = 1;

	if (pidfile) {
	    fprintf(pidfile, "%u\n", (u_int) getpid());
	    fclose(pidfile);
	}
    }

    info("symon version %s", SYMON_VERSION);

    if (flag_debug == 1)
	info("program id=%d", (u_int) getpid());

    /* setup signal handlers */
    signal(SIGALRM, alarmhandler);
    signal(SIGHUP, huphandler);
    signal(SIGINT, exithandler);
    signal(SIGQUIT, exithandler);
    signal(SIGTERM, exithandler);

    /* prepare crc32 */
    init_crc32();

    /* init modules */
    SLIST_FOREACH(mux, &mul, muxes) {
	connect2mux(mux);
	SLIST_FOREACH(stream, &mux->sl, streams) {
	    (streamfunc[stream->type].init) (stream->args);
	}
    }
    set_stream_use(&mul);

    /* setup alarm */
    timerclear(&alarminterval.it_interval);
    timerclear(&alarminterval.it_value);
    alarminterval.it_interval.tv_sec =
	alarminterval.it_value.tv_sec = symon_interval;

    if (setitimer(ITIMER_REAL, &alarminterval, NULL) != 0) {
	fatal("alarm setup failed: %.200s", strerror(errno));
    }

    for (;;) {			/* FOREVER */
	sleep(symon_interval);	/* alarm will interrupt sleep */

	if (flag_hup == 1) {
	    flag_hup = 0;

	    SLIST_INIT(&newmul);

	    if (!read_config_file(&newmul, cfgpath)) {
		info("new configuration contains errors; keeping old configuration");
		free_muxlist(&newmul);
	    } else {
		free_muxlist(&mul);
		mul = newmul;
		info("read configuration file '%.200s' successfully", cfgpath);

		/* init modules */
		SLIST_FOREACH(mux, &mul, muxes) {
		    connect2mux(mux);
		    SLIST_FOREACH(stream, &mux->sl, streams) {
			(streamfunc[stream->type].init) (stream->args);
		    }
		}
		set_stream_use(&mul);
	    }
	} else {
	    /* populate for modules that get all their measurements in one go */
	    for (i = 0; i < MT_EOT; i++)
		if (streamfunc[i].used && (streamfunc[i].gets != NULL))
		    (streamfunc[i].gets) ();

	    SLIST_FOREACH(mux, &mul, muxes) {
		prepare_packet(mux);

		SLIST_FOREACH(stream, &mux->sl, streams)
		    stream_in_packet(stream, mux);

		finish_packet(mux);

		send_packet(mux);
	    }
	}
    }

    return (EX_SOFTWARE);     /* NOTREACHED */
}
