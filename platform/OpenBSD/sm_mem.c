/*
 * $Id: sm_mem.c,v 1.1 2001/04/24 19:08:27 dijkstra Exp $
 *
 * Another quick try: get current memory load.
 */

#include <unistd.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/swap.h>

static int pageshift;
#define LOG1024 10
#define pagetok(size) ((size) << pageshift)
long memory_stats[4];

static int
swapmode(used, total)
     int    *used;
     int    *total;
{
  int     nswap, rnswap, i;
  struct swapent *swdev;
  
  nswap = swapctl(SWAP_NSWAP, 0, 0);
  if (nswap == 0)
    return 0;
  
  swdev = malloc(nswap * sizeof(*swdev));
  if (swdev == NULL)
    return 0;
  
  rnswap = swapctl(SWAP_STATS, swdev, nswap);
  if (rnswap == -1)
    return 0;
  
  /* if rnswap != nswap, then what? */
  
  /* Total things up */
  *total = *used = 0;
  for (i = 0; i < nswap; i++) {
    if (swdev[i].se_flags & SWF_ENABLE) {
      *used += (swdev[i].se_inuse / (1024 / DEV_BSIZE));
      *total += (swdev[i].se_nblks / (1024 / DEV_BSIZE));
    }
  }
  
  free(swdev);
  return 1;
}
int main(argc, argv)
     int argc;
     char *argv[];
{
  static int vmtotal_mib[] = {CTL_VM, VM_METER};
  struct vmtotal vmtotal;
  int total, i;
  size_t size;
  int pagesize;
  
  pagesize = sysconf( _SC_PAGESIZE );
  pageshift = 0;
  while (pagesize > 1) {
    pageshift++;
    pagesize >>= 1;
  }

  /* we only need the amount of log(2)1024 for our conversion */
  pageshift -= LOG1024;

  /* get total -- systemwide main memory usage structure */
  size = sizeof(vmtotal);
  for (;;) {
    if (sysctl(vmtotal_mib, 2, &vmtotal, &size, NULL, 0) < 0) {
      warn("sysctl failed");
      bzero(&vmtotal, sizeof(vmtotal));
    }
    /* convert memory stats to Kbytes */
    memory_stats[0] = pagetok(vmtotal.t_arm);
    memory_stats[1] = pagetok(vmtotal.t_rm);
    memory_stats[2] = pagetok(vmtotal.t_free);
    if (!swapmode(&memory_stats[3], &memory_stats[4])) {
      memory_stats[3] = 0;
      memory_stats[4] = 0;
    }
    printf("Real %d/%d, K act/tot, Free: %d Swap: %d/%d K used/tot\n", 
	   memory_stats[0], memory_stats[1], memory_stats[2], memory_stats[3], memory_stats[4] );
    sleep(1);
  }
}
