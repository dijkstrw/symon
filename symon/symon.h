/*
 * $Id: symon.h,v 1.7 2002/03/09 16:25:33 dijkstra Exp $
 *
 * Mon - a minimal system monitor
 * 
 * General (global) definitions
 */
#ifndef _MON_H
#define _MON_H

#include <kvm.h>
#include <nlist.h>
#include <sys/queue.h>

#include "lex.h"
#include "data.h"

/* Log base 2 of 1024 is 10 (2^10=1024) */
#define LOG1024		10

/* Number of seconds between measurement intervals */
#define MON_INTERVAL 1

/* kvm interface */
#ifdef MON_KVM
extern kvm_t *kvmd;
extern struct nlist mon_nl[];
#define MON_IFNET 0
#define MON_DL    1
extern int kread(u_long,char *,int);
#endif

/* prototypes */
__BEGIN_DECLS
/* cpu.c */
extern void init_cpu __P((char *));
extern int  get_cpu __P((char *, int, char *));

/* mem.c */
extern void init_mem __P((char *));
extern int  get_mem __P((char *, int, char *));

/* ifstat.c */
extern void init_if __P((char *));
extern int  get_if __P((char *, int, char *));

/* disk.c */
extern void init_io __P((char *));
extern int  get_io __P((char *, int, char *));
__END_DECLS

#endif /* _MON_H */
