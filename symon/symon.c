/*
 * $Id: symon.c,v 1.2 2001/04/29 13:08:48 dijkstra Exp $
 *
 * All configuration is done in the source. The main program
 * does not take any arguments (yet)
 */
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <err.h>
#include <rrd.h>
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
  { "cpu", NULL, "cpu.rrd", init_cpu, get_cpu, 5, 0},
  { "mem", NULL, "mem.rrd", init_mem, get_mem, 5, 0},
#ifdef MON_KVM
  { "ns",  "xl0", "if_de0.rrd", init_netstat,  get_netstat,  5, 0},
  { "ns",  "de0", "if_de0.rrd", init_netstat,  get_netstat,  5, 0},
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
  char *arg_ra[2];

#ifdef MON_KVM
  char *nlistf = NULL, *memf = NULL;

  /* Populate kernel name list */
  if ((kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, mon_buf)) == NULL) {
    errx(1,"kvm_open: %s", mon_buf);
    exit(1);
  }
  /* Release priviledges */
  setegid(getgid());
  setgid(getgid());
  /* Lookup in kernel namelist */
  if (kvm_nlist(kvmd, mon_nl) < 0 || mon_nl[0].n_type == 0) {
    if (nlistf)
      errx(1, "%s: no namelist", nlistf);
    else
      errx(1, "no namelist");
    exit(1);
  }
#endif

  i=-1;
  while (monm[++i].type) {
    (monm[i].init)(monm[i].arg);
  }
  for (;;) {  /* Forever */
    sleep(1);
    i=-1;
    while (monm[++i].type)
      if (++monm[i].sleep == monm[i].interval) 
	{
	  monm[i].sleep=0;
	  arg_ra[0]=monm[i].file;
	  arg_ra[1]=monm[i].file;
	  arg_ra[2]=monm[i].get(monm[i].arg);
	  printf( "%d:%s", rrd_update(3,arg_ra), rrd_get_error());
	}
  }
}
