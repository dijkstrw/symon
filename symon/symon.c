/*
 * $Id: symon.c,v 1.9 2002/03/09 16:25:33 dijkstra Exp $
 *
 * This program wakes up every MON_INTERVAL to measure cpu, memory and
 * interface statistics. This code takes care of waking up every so often,
 * while the actual measurement tasks are handled elsewhere.
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <varargs.h>

#include <rrd.h>

#include "mon.h"
#include "net.h"
#include "readconf.h"
#include "data.h"
#include "error.h"

struct muxlist muxlist = SLIST_HEAD_INITIALIZER(muxlist);

#ifdef MON_KVM
kvm_t *kvmd;
struct nlist mon_nl[] = {
    {"_ifnet"},      /* MON_IFNET = 0  (mon.h)*/
    {"_disklist"},   /* MON_DL    = 1  (mon.h)*/
    {""},
};

int kread(addr, buf, size)
    u_long addr;
    char *buf;
    int size;
{
    if (kvm_read(kvmd, addr, buf, size) != size) {
	fatal("kvm_read:%s", kvm_geterr(kvmd));
    }
    return (0);
}
#endif /* MON_KVM */

/* map stream types onto inits and getters */
struct {
    int type;
    void (*init)(char *);
    int (*get)(char*, int, char *);
} streamfunc[] = {
    {MT_IO,  init_io,  get_io},
    {MT_CPU, init_cpu, get_cpu},
    {MT_MEM, init_mem, get_mem},
    {MT_IF,  init_if,  get_if},
    {MT_EOT, NULL, NULL}
};

void alarmhandler() {
}

int main(argc, argv)
    int argc;
    char *argv[];
{
    int i;
    struct itimerval ai,ri;
    struct mux *mux;
    struct stream *stream;
    char mon_buf[_POSIX2_LINE_MAX];
    int offset;

#ifdef MON_KVM
    char *nlistf = NULL, *memf = NULL;

    /* Populate our kernel name list */
    if ((kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, mon_buf)) == NULL) {
	fatal("kvm_open: %s", mon_buf);
    }
    if (kvm_nlist(kvmd, mon_nl) < 0 || mon_nl[0].n_type == 0) {
	if (nlistf)
	    fatal("%s: no namelist", nlistf);
	else
	    fatal("no namelist");

	/*NOTREACHED*/
    }
#endif

    setegid(getgid());
    setgid(getgid());

    read_config_file("mon.conf");

#ifndef DEBUG
    if (daemon(0,0) != 0) {
	syslog(LOG_ALERT,"daemonize failed -- exiting");
	fatal("daemonize failed");
    }
#endif

    /* Init modules */
    syslog(LOG_INFO,"mon $Revision: 1.9 $ started");

    offset = 0;
    SLIST_FOREACH(mux, &muxlist, muxes) {
	SLIST_FOREACH(stream, &mux->sl, streams) {
	    (streamfunc[stream->type].init)(stream->args);
	}
    }

    /* Setup signal handlers */
    signal(SIGALRM, alarmhandler);
/*      signal(SIGINT, exit); */
/*      signal(SIGQUIT, exit); */
/*      signal(SIGTERM, exit); */
/*      signal(SIGXCPU, exit); */
/*      signal(SIGXFSZ, exit); */
/*      signal(SIGUSR1, exit); */
/*      signal(SIGUSR2, exit); */

    /* Setup alarm */
    timerclear(&ai.it_interval);
    timerclear(&ai.it_value);
    ai.it_interval.tv_sec=ai.it_value.tv_sec=MON_INTERVAL;
    if (setitimer(ITIMER_REAL, &ai, &ri) != 0) {
	syslog(LOG_INFO,"alarm setup failed -- %s", strerror(errno));
	exit(1);
    }

    for (;;) {  /* forever */
	/* sleep until the signal handler catches an alarm */
	sleep(MON_INTERVAL*2);

	/* fill mux->data with stream getters.
	   mux->offset stores the amount of data actually written */
	SLIST_FOREACH(mux, &muxlist, muxes) {
	    memset(mux->data, 0, _POSIX2_LINE_MAX);
	    mux->offset = 0;
	    SLIST_FOREACH(stream, &mux->sl, streams) {
		printf( "%s: pre=%4d, post=", stream->args, mux->offset);
		mux->offset += (streamfunc[stream->type].get)
		    (&mux->data[mux->offset], 
		     _POSIX2_LINE_MAX - mux->offset, 
		     stream->args);
		printf( "%4d\n", mux->offset);
	    }
	    warning("buffer mon_buf filled upto %d\n", mux->offset);
	}
    }
    /* NOTREACHED */
}

















