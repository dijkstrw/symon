/*
 * $Id: sm_cpu.c,v 1.2 2001/04/28 16:06:23 dijkstra Exp $
 *
 * Get current cpu load.
 */
#include <sys/param.h>
#include <sys/dkstat.h>
#include <sys/sysctl.h>
#include <limits.h>
#include <stdio.h>
#include <err.h>
#include "mon.h"

/*
 * globals for this module all start with cp_
 */
static int cp_time_mib[] = { CTL_KERN, KERN_CPTIME };
static size_t cp_size;
static long cp_time[CPUSTATES];
static long cp_old[CPUSTATES];
static long cp_diff[CPUSTATES];
static int cp_states[CPUSTATES];
static char cp_buf[_POSIX2_LINE_MAX];
/*
 *  percentages(cnt, out, new, old, diffs) - calculate percentage change
 *      between array "old" and "new", putting the percentages i "out".
 *      "cnt" is size of each array and "diffs" is used for scratch space.
 *      The array "old" is updated on each call.
 *      The routine assumes modulo arithmetic.  This function is especially
 *      useful on BSD mchines for calculating cpu state percentages.
 */
int percentages(cnt, out, new, old, diffs)
     int cnt;
     int *out;
     register long *new;
     register long *old;
     long *diffs;
{
  register int i;
  register long change;
  register long total_change;
  register long *dp;
  long half_total;

  /* initialization */
  total_change = 0;
  dp = diffs;

  /* calculate changes for each state and the overall change */
  for (i = 0; i < cnt; i++)
    {
      if ((change = *new - *old) < 0)
        {
	  /* this only happens when the counter wraps */
	  change = ((unsigned int)*new-(unsigned int)*old);
        }
      total_change += (*dp++ = change);
      *old++ = *new++;
    }
  
    /* avoid divide by zero potential */
  if (total_change == 0)
    {
      total_change = 1;
    }
  
  /* calculate percentages based on overall change, rounding up */
  half_total = total_change / 2l;
  for (i = 0; i < cnt; i++)
    {
      *out++ = ((*diffs++ * 1000 + half_total) / total_change);
    }
  
  /* return the total in case the caller wants to use it */
  return(total_change);
}

void init_cpu(s) 
     char *s;
{
  cp_size = sizeof(cp_time);
  /* Call get_cpu once to fill the cp_old structure */
  get_cpu(NULL);
}

char *get_cpu(s) 
     char *s;
{
  int total;
  if (sysctl(cp_time_mib, 2, &cp_time, &cp_size, NULL, 0) < 0) {
    warn("sysctl kern.cp_time failed");
    total = 0;
  }

  /* convert cp_time counts to percentages */
  total = percentages(CPUSTATES, cp_states, cp_time, cp_old, cp_diff);
    
  snprintf( &cp_buf[0], _POSIX2_LINE_MAX, 
	    "%d:%d:%d:%d:%d", 
	    cp_states[CP_USER], cp_states[CP_NICE], cp_states[CP_SYS], 
	    cp_states[CP_INTR], cp_states[CP_IDLE]);
  return &cp_buf[0];
}
