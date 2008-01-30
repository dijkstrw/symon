/* $Id: symon.h,v 1.37 2008/01/30 12:06:50 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2005 Willem Dijkstra
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

#ifndef _SYMON_SYMON_H
#define _SYMON_SYMON_H

#include "lex.h"
#include "data.h"

#define SYMON_PID_FILE "/var/run/symon.pid"
#define SYMON_DEFAULT_INTERVAL 5        /* measurement interval */

/* funcmap holds functions to be called for the individual monitors:
 *
 * - privinit = priviledged init, called before chroot
 * - init     = called once, right after configuration
 * - gets     = called every monitor interval, can be used by modules that get
 *              all their measurements in one go.
 * - get      = obtain measurement
 */
struct funcmap {
    int type;
    int used;
    void (*privinit) ();
    void (*init) (struct stream *);
    void (*gets) ();
    int (*get) (char *, int, struct stream *);
};
extern struct funcmap streamfunc[];

extern int symon_interval;

/* prototypes */
__BEGIN_DECLS
/* sm_cpu.c */
extern void init_cpu(struct stream *);
extern void gets_cpu();
extern int get_cpu(char *, int, struct stream *);

/* sm_cpuiow.c */
extern void init_cpuiow(struct stream *);
extern void gets_cpuiow();
extern int get_cpuiow(char *, int, struct stream *);

/* sm_mem.c */
extern void init_mem(struct stream *);
extern void gets_mem();
extern int get_mem(char *, int, struct stream *);

/* sm_if.c */
extern void init_if(struct stream *);
extern void gets_if();
extern int get_if(char *, int, struct stream *);

/* sm_io.c */
extern void init_io(struct stream *);
extern void gets_io();
extern int get_io(char *, int, struct stream *);

/* sm_pf.c */
extern void privinit_pf();
extern void init_pf(struct stream *);
extern void gets_pf();
extern int get_pf(char *, int, struct stream *);

/* sm_pfq.c */
extern void privinit_pfq();
extern void init_pfq(struct stream *);
extern void gets_pfq();
extern int get_pfq(char *, int, struct stream *);

/* sm_mbuf.c */
extern void init_mbuf(struct stream *);
extern int get_mbuf(char *, int, struct stream *);

/* sm_debug.c */
extern void init_debug(struct stream *);
extern int get_debug(char *, int, struct stream *);

/* sm_proc.c */
extern void privinit_proc();
extern void init_proc(struct stream *);
extern void gets_proc();
extern int get_proc(char *, int, struct stream *);

/* sm_sensor.c */
extern void privinit_sensor();
extern void init_sensor(struct stream *);
extern int get_sensor(char *, int, struct stream *);

/* sm_df.c */
extern void init_df(struct stream *);
extern void gets_df();
extern int get_df(char *, int, struct stream *);
__END_DECLS

#endif                          /* _SYMON_SYMON_H */
