/* $Id: symon.h,v 1.34 2005/09/30 14:04:30 dijkstra Exp $ */

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
#define SYMON_DEFAULT_INTERVAL 5	/* measurement interval */
#define SYMON_WARN_SENDERR 50	        /* warn every x errors */
#define SYMON_MAX_DOBJECTS 10000	/* max dynamic allocation; local limit per
					 * measurement module */
#define SYMON_MAX_OBJSIZE (_POSIX2_LINE_MAX)
#define SYMON_SENSORMASK 0xFF           /* sensors 0-255 are allowed */
#define SYMON_MAXCPUS 4

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
    void (*init) (char *);
    void (*gets) ();
    int (*get) (char *, int, char *);
};
extern struct funcmap streamfunc[];

extern int symon_interval;

/* prototypes */
__BEGIN_DECLS
/* sm_cpu.c */
extern void init_cpu(char *);
extern void gets_cpu();
extern int get_cpu(char *, int, char *);

/* sm_mem.c */
extern void init_mem(char *);
extern void gets_mem();
extern int get_mem(char *, int, char *);

/* sm_if.c */
extern void init_if(char *);
extern void gets_if();
extern int get_if(char *, int, char *);

/* sm_io.c */
extern void init_io(char *);
extern void gets_io();
extern int get_io(char *, int, char *);

/* sm_pf.c */
extern void privinit_pf();
extern void init_pf(char *);
extern int get_pf(char *, int, char *);

/* sm_pfq.c */
extern void privinit_pfq();
extern void init_pfq(char *);
extern void gets_pfq();
extern int get_pfq(char *, int, char *);

/* sm_mbuf.c */
extern void init_mbuf(char *);
extern int get_mbuf(char *, int, char *);

/* sm_debug.c */
extern void init_debug(char *);
extern int get_debug(char *, int, char *);

/* sm_proc.c */
extern void init_proc(char *);
extern void gets_proc();
extern int get_proc(char *, int, char *);

/* sm_sensor.c */
extern void privinit_sensor();
extern void init_sensor(char *);
extern int get_sensor(char *, int, char *);
__END_DECLS

#endif				/* _SYMON_SYMON_H */
