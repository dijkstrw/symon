/*
 * $Id: symon.c,v 1.3 2001/04/30 14:27:09 dijkstra Exp $
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
#include <syslog.h>
#include <varargs.h>
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
  int errs=0;
  char *arg_ra[2];

#ifdef MON_KVM
  char *nlistf = NULL, *memf = NULL;

  /* Populate kernel name list */
  if ((kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, mon_buf)) == NULL) {
    errx(1,"kvm_open: %s", mon_buf);
    exit(1);
  }
  /* Lookup in kernel namelist */
  if (kvm_nlist(kvmd, mon_nl) < 0 || mon_nl[0].n_type == 0) {
    if (nlistf)
      errx(1, "%s: no namelist", nlistf);
    else
      errx(1, "no namelist");
    exit(1);
  }
#endif
  /* Release privs */
  setegid(getgid());
  setgid(getgid());

  if ( daemon(0,0) != 0 ) {
    syslog(LOG_ALERT,"daemonize failed -- exiting");
    errx(1, "daemonize failed");
  }
  /* Init modules */
  syslog(LOG_INFO,"system monitor $Revision: 1.3 $ started");
  i=-1;
  while (monm[++i].type) {
    syslog(LOG_INFO,"starting module %s(%s)", monm[i].type, monm[i].arg);
    (monm[i].init)(monm[i].arg);
  }
  /* Loop forever */
  for (;;) {  
    sleep(1);
    i=-1;
    while (monm[++i].type)
      if (++monm[i].sleep == monm[i].interval) 
	{
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

    if (errs>10) {
      syslog(LOG_ALERT,"counted too many errors (%d) -- quitting.",errs);
      exit(1);
    }
  } 
  /* NOTREACHED */
}
