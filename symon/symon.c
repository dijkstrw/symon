/*
 * $Id: symon.c,v 1.4 2001/05/02 21:38:38 dijkstra Exp $
 *
 * All configuration is done in the source. The main program
 * does not take any arguments (yet)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <err.h>
#include <rrd.h>
#include <time.h>
#include <syslog.h>
#include <varargs.h>
#include <sys/param.h>
#include "mon.h"

#ifdef MON_KVM
kvm_t *kvmd;
struct nlist mon_nl[] = {
  {"_ifnet"}, /* MON_IFNET = 0  (mon.h)*/
  {""},
};
#endif
char mon_buf[_POSIX2_LINE_MAX];

struct monm monm[] = {
  { "cpu", "0", "/home/dijkstra/project/mon/cpu.rrd", init_cpu, get_cpu, 5, 0},  /* arg 0 is not used */
  { "mem", "all", "/home/dijkstra/project/mon/mem.rrd", init_mem, get_mem, 5, 0}, /* arg all is not used */
#ifdef MON_KVM
  { "ifstat",  "xl0", "/home/dijkstra/project/mon/if_xl0.rrd", init_ifstat,  get_ifstat,  5, 0},
  { "ifstat",  "de0", "/home/dijkstra/project/mon/if_de0.rrd", init_ifstat,  get_ifstat,  5, 0},
#endif
  { NULL, NULL, NULL, NULL, NULL, 0, 0}
};

#ifdef MON_KVM
int kread(addr, buf, size)
     u_long addr;
     char *buf;
     int size;
{
  if (kvm_read(kvmd, addr, buf, size) != size) {
    errx(1, "%s", kvm_geterr(kvmd));
    return (-1);
  }
  return (0);
}
#endif

int main(argc, argv)
     int argc;
     char *argv[];
{
  int i;
  int errs=0;                   /* total errors during rrd_updates */
  char *arg_ra[2];              /* arguments used to call rrd_update */
  struct timespec reqs, rems;   /* requested sleep, remaining sleep */
  struct timeval startt,endt,difft;   /* start timeval, end timeval, difference */
  int slept;                    /* actual time spent sleeping in seconds */

#ifdef MON_KVM
  char *nlistf = NULL, *memf = NULL;

  /* Populate our kernel name list */
  if ((kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, mon_buf)) == NULL) {
    errx(1,"kvm_open: %s", mon_buf);
    exit(1);
  }
  if (kvm_nlist(kvmd, mon_nl) < 0 || mon_nl[0].n_type == 0) {
    if (nlistf)
      errx(1, "%s: no namelist", nlistf);
    else
      errx(1, "no namelist");
    exit(1);
  }
#endif
  /* Release privs and daemonize */
  setegid(getgid());
  setgid(getgid());
  if (daemon(0,0) != 0) {
    syslog(LOG_ALERT,"daemonize failed -- exiting");
    errx(1, "daemonize failed");
  }

  /* Init modules */
  syslog(LOG_INFO,"system monitor $Revision: 1.4 $ started");
  reqs.tv_sec=monm[0].interval; /* Needs a valid seed to start */
  reqs.tv_nsec=0;
  i=-1;
  while (monm[++i].type) {
    syslog(LOG_INFO,"starting module %s(%s)", monm[i].type, monm[i].arg);
    reqs.tv_sec=MIN(reqs.tv_sec,monm[i].interval);
    (monm[i].init)(monm[i].arg);
  }

  /* initial call to gettimeofday returns weird values! */
  gettimeofday(&startt,NULL);

  timerclear(&difft);
  slept = reqs.tv_sec;  /* invariant slept */

  for (;;) {  /* forever */
    /* Sleep for a while (reqs seconds). 
       nanosleep is used to adjust for "timedrift" introduced by the monitoring
       modules during data gathering phase. */
    do {
      //      printf("Going to sleep for %u.%lu\n",reqs.tv_sec,reqs.tv_nsec);
      nanosleep(&reqs,&rems);
      timespecsub(&reqs,&rems,&reqs);
    } while (rems.tv_nsec || rems.tv_sec );
    
    /* Check whether a module needs to gather data and compute time that needs
       to be slept before the next activity */
    gettimeofday(&startt,NULL);
    i=-1;
    reqs.tv_sec = monm[1].interval; /* seed */
    while (monm[++i].type) {
      monm[i].sleep += slept;
      if (monm[i].sleep >= monm[i].interval) {
	monm[i].sleep=0;
	arg_ra[0]=monm[i].file; /* should be rrd_update */
	arg_ra[1]=monm[i].file;
	arg_ra[2]=monm[i].get(monm[i].arg);
	rrd_update(3,arg_ra);
	if (rrd_test_error()) {
	  syslog(LOG_INFO,"rrd_update:%s",rrd_get_error());
	  rrd_clear_error();                                                            
	  errs++;
	}
      }
      reqs.tv_sec = MIN(reqs.tv_sec,(monm[i].interval-monm[i].sleep));
      if (errs>10) {
	syslog(LOG_ALERT,"counted too many rrd errors (%d) -- quitting.",errs);
	exit(1);
      }
    }
    gettimeofday(&endt,NULL);

    /* Compute time spent gathering statistics.  The new sleeping time is the
       minimum time until next activity minus the time spent
       gathering. (resolution is in microseconds)*/
    slept = reqs.tv_sec;
    timersub(&endt,&startt,&difft);
    //    printf("Spent %lu.%lu seconds gathering\n",difft.tv_sec,difft.tv_usec);

    reqs.tv_sec -= difft.tv_sec;
    reqs.tv_nsec = difft.tv_usec * -1000;
    while (reqs.tv_nsec < 0) {
      reqs.tv_sec--; reqs.tv_nsec += 1000000000L;
    }
    if (reqs.tv_sec < 0) { 
      /* Uh oh, we already spent all of our sleeping time gathering data. Reset
	 those modules involved and report the incident.  */
      slept += (reqs.tv_sec * -1);
      if (reqs.tv_nsec) slept--; /* there are some nanoseconds left in the
                                    current second to work with */
      i=-1;
      reqs.tv_sec = monm[1].interval; /* seed */
      reqs.tv_nsec = 0;
      while (monm[++i].type) {
	monm[i].sleep += slept;
	if (monm[i].sleep >= monm[i].interval) {
	  monm[i].sleep = 0;
	  syslog(LOG_INFO,"time gathering > sleeping time! skipped module %s(%s)",
		 monm[i].type, monm[i].arg);
	}
	reqs.tv_sec=MIN(reqs.tv_sec,(monm[i].interval-monm[i].sleep));
      }
      /* All the overrun timers have been reset and a new inactivity time has
         been calculated. */
      slept = reqs.tv_sec;
    } 
  }
  /* NOTREACHED */
}
