/*
 * $Id: sm_cpu.c,v 1.1 2001/04/23 20:02:56 dijkstra Exp $
 *
 * Another quick try: get current cpu load.
 */
#include <sys/param.h>
#include <sys/dkstat.h>
#include <sys/sysctl.h>

static long cp_time[CPUSTATES];
static long cp_old[CPUSTATES];
static long cp_diff[CPUSTATES];
int     cpu_states[CPUSTATES];

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

int main(argc, argv)
     int argc;
     char *argv[];
{
  static int cp_time_mib[] = { CTL_KERN, KERN_CPTIME };
  int total, i;
  size_t size;

  size = sizeof(cp_time);

  for (;;) {
    if (sysctl(cp_time_mib, 2, &cp_time, &size, NULL, 0) < 0) {
      warn("sysctl kern.cp_time failed");
      total = 0;
    }

    /* convert cp_time counts to percentages */
    total = percentages(CPUSTATES, cpu_states, cp_time, cp_old, cp_diff);
    
    printf("user %d, nice %d, system %d, interrupt %d, idle %d\n", 
	   cpu_states[CP_USER], cpu_states[CP_NICE], cpu_states[CP_SYS], 
	   cpu_states[CP_INTR], cpu_states[CP_IDLE]);
    sleep(1);
  }

}
