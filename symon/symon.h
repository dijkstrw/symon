/* $Id: symon.h,v 1.17 2002/09/10 18:32:58 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2002 Willem Dijkstra
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _MON_MON_H
#define _MON_MON_H

#include <sys/queue.h>

#ifdef MON_KVM
#include <kvm.h>
#include <nlist.h>
#endif

#include "lex.h"
#include "data.h"

#define MON_CONFIG_FILE "/etc/mon.conf"
#define MON_PID_FILE    "/var/run/mon.pid"
#define MON_VERSION     "2.4"
#define MON_INTERVAL 5                           /* measurement interval */
#define MON_WARN_SENDERR 50                      /* warn every x errors */

/* kvm interface */
#ifdef MON_KVM
extern kvm_t *kvmd;
extern struct nlist mon_nl[];
#define MON_DL    0
#endif /* MON_KVM */

struct funcmap {
    int type;
    void (*init)(char *);
    int (*get)(char*, int, char *);
};

extern struct funcmap streamfunc[];

/* prototypes */
__BEGIN_DECLS
#ifdef MON_KVM
extern int kread(u_long, char *, int);
#else
extern int kread(u_long, char *, int); /* This is a stub that reports an error
					  when called. I included it so as not
					  to miss any stuff when the time has
					  come to rip all MON_KVM stuff out */
#endif

/* cpu.c */
extern void init_cpu(char *);
extern int  get_cpu(char *, int, char *);

/* mem.c */
extern void init_mem(char *);
extern int  get_mem(char *, int, char *);

/* if.c */
extern void init_if(char *);
extern int  get_if(char *, int, char *);

/* io.c */
extern void init_io(char *);
extern int  get_io(char *, int, char *);

/* pf.c */
extern void init_pf(char *);
extern int  get_pf(char *, int, char *);
__END_DECLS

#endif /*_MON_MON_H*/
