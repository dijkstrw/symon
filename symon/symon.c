/*
 * $Id: symon.c,v 1.7 2001/07/01 12:50:24 dijkstra Exp $
 *
 * All configuration is done in the source. The main program
 * does not take any arguments (yet)
 *
 * This program wakes up every MON_INTERVAL to measure cpu, memory and
 * interface statistics. This code takes care of waking up every so ofte, while
 * the actual measurement tasks are handled elsewhere.
 * 
 */
#include <err.h>
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

char mon_buf[_POSIX2_LINE_MAX];

struct monm monm[] = {
    {"cpu", "0", "/home/dijkstra/project/mon/cpu.rrd", init_cpu, get_cpu},  /* arg is not used */
    {"mem", "real+swap", "/home/dijkstra/project/mon/mem.rrd", init_mem, get_mem}, /* arg is not used */
#ifdef MON_KVM
    {"ifstat",  "xl0", "/home/dijkstra/project/mon/if_xl0.rrd", init_ifstat,  get_ifstat},
    {"ifstat",  "de0", "/home/dijkstra/project/mon/if_de0.rrd", init_ifstat,  get_ifstat},
    {"ifstat",  "wi0", "/home/dijkstra/project/mon/if_wi0.rrd", init_ifstat,  get_ifstat},
    {"disk",    "wd0", "/home/dijkstra/project/mon/disk_wd0.rrd", init_disk,  get_disk},
    {"disk",    "wd1", "/home/dijkstra/project/mon/disk_wd1.rrd", init_disk,  get_disk},
    {"disk",    "wd2", "/home/dijkstra/project/mon/disk_wd2.rrd", init_disk,  get_disk},
    {"disk",    "ccd0", "/home/dijkstra/project/mon/disk_ccd0.rrd", init_disk,  get_disk},
    {"disk",    "ccd1", "/home/dijkstra/project/mon/disk_ccd1.rrd", init_disk,  get_disk},
    {"disk",    "cd0", "/home/dijkstra/project/mon/disk_cd0.rrd", init_disk,  get_disk},
    {"disk",    "cd1", "/home/dijkstra/project/mon/disk_cd1.rrd", init_disk,  get_disk},
#endif
    { NULL, NULL, NULL, NULL, NULL}
};

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
	errx(1, "%s", kvm_geterr(kvmd));
    }
    return (0);
}
#endif

void alarmhandler() {
}

int main(argc, argv)
    int argc;
    char *argv[];
{
    int i;
    char *arg_ra[2];
    struct itimerval ai,ri;

#ifdef MON_KVM
    char *nlistf = NULL, *memf = NULL;

    /* Populate our kernel name list */
    if ((kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, mon_buf)) == NULL) {
	errx(1,"kvm_open: %s", mon_buf);
    }
    if (kvm_nlist(kvmd, mon_nl) < 0 || mon_nl[0].n_type == 0) {
	if (nlistf)
	    errx(1, "%s: no namelist", nlistf);
	else
	    errx(1, "no namelist");

	/*NOTREACHED*/
    }
#endif

    setegid(getgid());
    setgid(getgid());

#ifndef DEBUG
    if (daemon(0,0) != 0) {
	syslog(LOG_ALERT,"daemonize failed -- exiting");
	errx(1, "daemonize failed");
    }
#endif

    /* Init modules */
    syslog(LOG_INFO,"system monitor $Revision: 1.7 $ started");

    i=-1;
    while (monm[++i].type) {
	syslog(LOG_INFO,"starting module %s(%s)", monm[i].type, monm[i].arg);
	(monm[i].init)(monm[i].arg);
    }

    /* Setup signal handlers */
    signal(SIGALRM, alarmhandler);
    signal(SIGINT, exit);
    signal(SIGQUIT, exit);
    signal(SIGTERM, exit);
    signal(SIGXCPU, exit);
    signal(SIGXFSZ, exit);
    signal(SIGUSR1, exit);
    signal(SIGUSR2, exit);

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

	/* run past the modules */
	i=-1;
	while (monm[++i].type) {
	    arg_ra[0]=monm[i].file; /* should be rrd_update */
	    arg_ra[1]=monm[i].file;
	    arg_ra[2]=monm[i].get(monm[i].arg);
#ifdef DEBUG
	    syslog(LOG_INFO,"%s(%s)='%s'",monm[i].type,monm[i].arg,&mon_buf[0]);
#else
	    rrd_update(3,arg_ra);

	    if (rrd_test_error()) {
		syslog(LOG_INFO,"rrd_update:%s",rrd_get_error());
		rrd_clear_error();                                                            
	    }
#endif
	}
    }
    /* NOTREACHED */
}

