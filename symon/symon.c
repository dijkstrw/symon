/*
 * $Id: symon.c,v 1.5 2001/05/19 14:24:35 dijkstra Exp $
 *
 * All configuration is done in the source. The main program
 * does not take any arguments (yet)
 *
 * Two semaphores are used:
 * S_STARTMEASUREMENT and S_STOPMEASUREMENT. The main program -- with the
 * measurement loop -- blocks on START until the alarm handler removes the
 * block. The measurement loop will run and unblock STOP when finished. The
 * alarm handler checks STOP to determine wether the measurement is finished
 * before the next START is unblocked.
 * 
 */
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/param.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <varargs.h>
#include <rrd.h>
#include "mon.h"

char mon_buf[_POSIX2_LINE_MAX];

struct monm monm[] = {
    {"cpu", "0", "/home/dijkstra/project/mon/cpu.rrd", init_cpu, get_cpu},  /* arg is not used */
#ifdef MEM_SWAP
    {"mem", "real+swap", "/home/dijkstra/project/mon/mem.rrd", init_mem, get_mem}, /* arg is not used */
#else
    {"mem", "real", "/home/dijkstra/project/mon/mem.rrd", init_mem, get_mem}, /* arg is not used */
#endif  
#ifdef MON_KVM
    {"ifstat",  "xl0", "/home/dijkstra/project/mon/if_xl0.rrd", init_ifstat,  get_ifstat},
    {"ifstat",  "de0", "/home/dijkstra/project/mon/if_de0.rrd", init_ifstat,  get_ifstat},
#endif
    { NULL, NULL, NULL, NULL, NULL}
};

#ifdef MON_KVM
kvm_t *kvmd;
struct nlist mon_nl[] = {
    {"_ifnet"}, /* MON_IFNET = 0  (mon.h)*/
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

int sem=0;
struct sembuf asb;

void releasesem() {
    if (sem)
	semctl(sem,1,IPC_RMID,0);
}

void alarmhandler() {
    asb.sem_num=S_STOPMEASURE;
    asb.sem_flg=IPC_NOWAIT;
    asb.sem_op=-1;
    if (semop(sem,&asb,1)) {
	syslog(LOG_ALERT,"alarm before end of measurements -- skipping %d seconds", 
	       MON_INTERVAL);
    } else {
	asb.sem_num=S_STARTMEASURE;
	asb.sem_flg=IPC_NOWAIT;
	asb.sem_op=1;
	if (semop(sem,&asb,1)) {
	    syslog(LOG_INFO,"alarm: sem failed -- %s", strerror(errno));
	}
    }
}

int main(argc, argv)
    int argc;
    char *argv[];
{
    int i;
    char *arg_ra[2];
    struct itimerval ai,ri;
    struct sembuf sembuf;

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
    syslog(LOG_INFO,"system monitor $Revision: 1.5 $ started");

    i=-1;
    while (monm[++i].type) {
	syslog(LOG_INFO,"starting module %s(%s)", monm[i].type, monm[i].arg);
	(monm[i].init)(monm[i].arg);
    }

    /* Setup semaphore */
    if ((sem = semget(IPC_PRIVATE,2,(SEM_A|SEM_R)))==0) {
	syslog(LOG_INFO,"could not obtain semaphore -- %s", strerror(errno));
	exit(1);
    }
    syslog(LOG_INFO,"obtained semaphore w/id:%d",sem);
    atexit(releasesem);

    if (semctl(sem,0,SETVAL,S_STARTMEASURE)!=0) {
	syslog(LOG_INFO,"could not set semaphore -- %s", strerror(errno));
	exit(1);
    }
    if (semctl(sem,1,SETVAL,S_STOPMEASURE)!=0) {
	syslog(LOG_INFO,"could not set semaphore -- %s", strerror(errno));
	exit(1);
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
	sembuf.sem_num=S_STARTMEASURE;
	sembuf.sem_flg=0;
	sembuf.sem_op=-1;
	while (semop(sem,&sembuf,1)!=0) {
	    /* semop will return with an EINTR if it is interrupted by a
               signal. This happens each MON_INTERVAL with us!
               (sys/kern/sysv_sem.c) */
	    if (errno!=EINTR) {
		syslog(LOG_INFO,"measurement start: sem failed -- %s", strerror(errno));
		exit(1);
	    } 
	}

	/* run past the modules */
	i=-1;
	while (monm[++i].type) {
	    arg_ra[0]=monm[i].file; /* should be rrd_update */
	    arg_ra[1]=monm[i].file;
	    arg_ra[2]=monm[i].get(monm[i].arg);
#ifdef DEBUG
	    syslog(LOG_INFO,"%s(%s)='%s'",monm[i].type,monm[i].arg,&mon_buf[0]);
#endif
	    rrd_update(3,arg_ra);
	    if (rrd_test_error()) {
		syslog(LOG_INFO,"rrd_update:%s",rrd_get_error());
		rrd_clear_error();                                                            
	    }
	}

	sembuf.sem_num=S_STOPMEASURE;
	sembuf.sem_flg=IPC_NOWAIT;
	sembuf.sem_op=1;
	while (semop(sem,&sembuf,1)) {
	    syslog(LOG_ALERT,"measurement stop: sem failed -- %s", strerror(errno));
	    exit(1);
	}
    }
    /* NOTREACHED */
}

