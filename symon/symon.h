/*
 * $Id: symon.h,v 1.3 2001/04/30 14:27:09 dijkstra Exp $
 *
 * Mon - a minimal system monitor
 * 
 * General (global) definitions
 */
#include <kvm.h>
#include <nlist.h>

/* Log base 2 of 1024 is 10 (2^10=1024) */
#define LOG1024		10

/* Buffer used for reporting status from subsystems */
extern char mon_buf[];

/* Monitor subsystem structure */
struct monm {
  char* type;
  char* arg;
  char* file;
  void  (*init)(char *);
  char* (*get)(char *);
  int   interval;
  int   sleep;
};

/* kvm interface */
#ifdef MON_KVM
extern kvm_t *kvmd;
extern struct nlist mon_nl[];
#define MON_IFNET 0
extern int kread(u_long,char *,int);
#endif

/* cpu.c */
extern void init_cpu(char *);
extern char *get_cpu(char *);

/* mem.c */
extern void init_mem(char *);
extern char *get_mem(char *);

/* ifstat.c */
extern void init_ifstat(char *);
extern char *get_ifstat(char *);

