/*
 * $Id: symon.c,v 1.1 2001/04/28 16:06:44 dijkstra Exp $
 *
 * All configuration is done in the source. The main program
 * does not take any arguments (yet)
 */
#include <stdio.h>
#include <unistd.h>
#include "mon.h"
struct monm monm[] = {
  { "cpu", NULL, "cpu.rrd", init_cpu, get_cpu, 5, 0},
  { "mem", NULL, "mem.rrd", init_mem, get_mem, 5, 0},
  //  { "ns",  "de0", "if_de0.rrd", init_ns,  get_ns,  5, 0},
  //  { "ns",  "xl0", "if_de0.rrd", init_ns,  get_ns,  5, 0},
  { NULL, NULL, NULL, NULL, NULL, 0, 0}
};

int main(argc, argv)
     int argc;
     char *argv[];
{
  int i=-1;
  char *buf;
  while (monm[++i].type) {
    (monm[i].init)(monm[i].arg);
  }
  for (;;) {  /* Forever */
    sleep(1);
    i=-1;
    while (monm[++i].type)
      if (++monm[i].sleep == monm[i].interval) 
	{
	  monm[i].sleep=0; buf = (monm[i].get(monm[i].arg));
	  printf("%s:%s\n", monm[i].type, buf);
	}
  }
}
