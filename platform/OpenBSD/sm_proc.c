/*
 * Copyright (c) 2001-2012 Willem Dijkstra
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

/*
 * Get process statistics from kernel and return them in symon_buf as
 *
 * number of processes : ticks_user : ticks_system : ticks_interrupt :
 * cpuseconds : procsizes : resident segment sizes
 *
 */

#include "conf.h"

#include <sys/param.h>
#include <sys/sysctl.h>

#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "symon.h"
#include "xmalloc.h"

#define pagetob(size) (((u_int32_t)size) << proc_pageshift)

#ifdef HAS_KERN_PROC2
#define KINFO_NEWAPI
#define KINFO_MIB KERN_PROC2
#define KINFO_STRUCT kinfo_proc2
#else
#ifndef HAS_KERN_KPPROC
#define KINFO_NEWAPI
#define KINFO_MIB KERN_PROC
#define KINFO_STRUCT kinfo_proc
#else
#undef KINFO_NEWAPI
#define KINFO_STRUCT kinfo_proc
#endif
#endif

/* Globals for this module start with proc_ */
static struct KINFO_STRUCT *proc_ps = NULL;
static int proc_max = 0;
static int proc_cur = 0;
static int proc_stathz = 0;
static int proc_pageshift;
static int proc_pagesize;

/* get scale factor cpu percentage counter */
#define FIXED_PCTCPU FSCALE
typedef long pctcpu;
#define pctdouble(p) ((double)(p) / FIXED_PCTCPU)


void
gets_proc()
{
    int mib[6];
    int procs;
    size_t size;

    /* how much memory is needed */
    mib[0] = CTL_KERN;
    mib[1] = KERN_NPROCS;
    size = sizeof(procs);
    if (sysctl(mib, 2, &procs, &size, NULL, 0) < 0) {
        fatal("%s:%d: sysctl failed: can't get kern.nproc",
              __FILE__, __LINE__);
    }

#ifdef KINFO_NEWAPI
    /* increase buffers if necessary */
    if (procs > proc_max) {
        proc_max = (procs * 5) / 4;

        if (proc_max > SYMON_MAX_DOBJECTS) {
            fatal("%s:%d: dynamic object limit (%d) exceeded for kinfo_proc structures",
                  __FILE__, __LINE__, SYMON_MAX_DOBJECTS);
        }

        proc_ps = xrealloc(proc_ps, proc_max * sizeof(struct KINFO_STRUCT));
    }

    /* read data in anger */
    mib[0] = CTL_KERN;
    mib[1] = KINFO_MIB;
    mib[2] = KERN_PROC_KTHREAD;
    mib[3] = 0;
    mib[4] = sizeof(struct KINFO_STRUCT);
    mib[5] = proc_max;
    size = proc_max * sizeof(struct KINFO_STRUCT);
    if (sysctl(mib, 6, proc_ps, &size, NULL, 0) < 0) {
        warning("proc probe cannot get processes");
        proc_cur = 0;
        return;
    }

    if (size % sizeof(struct KINFO_STRUCT) != 0) {
        warning("proc size mismatch: got %d bytes, not dividable by sizeof(kinfo_proc) %d",
                size, sizeof(struct KINFO_STRUCT));
        proc_cur = 0;
    } else {
        proc_cur = size / sizeof(struct KINFO_STRUCT);
    }
#else
    /* increase buffers if necessary */
    if (procs > proc_max) {
        proc_max = (procs * 5) / 4;

        if (proc_max > SYMON_MAX_DOBJECTS) {
            fatal("%s:%d: dynamic object limit (%d) exceeded for kinfo_proc structures",
                  __FILE__, __LINE__, SYMON_MAX_DOBJECTS);
        }

        proc_ps = xrealloc(proc_ps, proc_max * sizeof(struct kinfo_proc));
    }

    /* read data in anger */
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_KTHREAD;
    size = proc_max * sizeof(struct kinfo_proc);
    if (sysctl(mib, 3, proc_ps, &size, NULL, 0) < 0) {
        warning("proc probe cannot get processes");
        proc_cur = 0;
        return;
    }

    if (size % sizeof(struct kinfo_proc) != 0) {
        warning("proc size mismatch: got %d bytes, not dividable by sizeof(kinfo_proc) %d",
                size, sizeof(struct kinfo_proc));
        proc_cur = 0;
    } else {
        proc_cur = size / sizeof(struct kinfo_proc);
    }
#endif
}

void
privinit_proc()
{
    /* EMPTY */
}

void
init_proc(struct stream *st)
{
    int mib[2] = {CTL_KERN, KERN_CLOCKRATE};
    struct clockinfo cinf;
    size_t size = sizeof(cinf);

    /* get clockrate */
    if (sysctl(mib, 2, &cinf, &size, NULL, 0) == -1) {
        fatal("%s:%d: could not get clockrate", __FILE__, __LINE__);
    }

    proc_stathz = cinf.stathz;

    /* get pagesize */
    proc_pagesize = sysconf(_SC_PAGESIZE);
    proc_pageshift = 0;
    while (proc_pagesize > 1) {
        proc_pageshift++;
        proc_pagesize >>= 1;
    }

    info("started module proc(%.200s)", st->arg);
}

int
get_proc(char *symon_buf, int maxlen, struct stream *st)
{
    int i;
    struct KINFO_STRUCT *pp;
    u_quad_t  cpu_ticks = 0;
    u_quad_t  cpu_uticks = 0;
    u_quad_t  cpu_iticks = 0;
    u_quad_t  cpu_sticks =0;
    u_int32_t cpu_secs = 0;
    double    cpu_pct = 0;
    double    cpu_pcti = 0;
    u_int32_t mem_procsize = 0;
    u_int32_t mem_rss = 0;
    int n = 0;

    for (pp = proc_ps, i = 0; i < proc_cur; pp++, i++) {
#ifdef KINFO_NEWAPI
         if (strncmp(st->arg, pp->p_comm, strlen(st->arg)) == 0) {
             /* cpu time - accumulated */
             cpu_uticks += pp->p_uticks;  /* user */
             cpu_sticks += pp->p_sticks;  /* sys  */
             cpu_iticks += pp->p_iticks;  /* int  */
             /* cpu time - percentage since last measurement */
             cpu_pct = pctdouble(pp->p_pctcpu) * 100.0;
             cpu_pcti += cpu_pct;
             /* memory size - shared pages are counted multiple times */
             mem_procsize += pagetob(pp->p_vm_tsize + /* text pages */
                                     pp->p_vm_dsize + /* data */
                                     pp->p_vm_ssize); /* stack */
             mem_rss += pagetob(pp->p_vm_rssize);     /* rss  */
#else
         if (strncmp(st->arg, pp->kp_proc.p_comm, strlen(st->arg)) == 0) {
             /* cpu time - accumulated */
             cpu_uticks += pp->kp_proc.p_uticks;  /* user */
             cpu_sticks += pp->kp_proc.p_sticks;  /* sys  */
             cpu_iticks += pp->kp_proc.p_iticks;  /* int  */
             /* cpu time - percentage since last measurement */
             cpu_pct = pctdouble(pp->kp_proc.p_pctcpu) * 100.0;
             cpu_pcti += cpu_pct;
             /* memory size - shared pages are counted multiple times */
             mem_procsize += pagetob(pp->kp_eproc.e_vm.vm_tsize + /* text pages */
                                     pp->kp_eproc.e_vm.vm_dsize + /* data */
                                     pp->kp_eproc.e_vm.vm_ssize); /* stack */
             mem_rss += pagetob(pp->kp_eproc.e_vm.vm_rssize);     /* rss  */
#endif
             n++;
         }
    }

    /* calc total cpu_secs spent */
    cpu_ticks = cpu_uticks + cpu_sticks + cpu_iticks;
    cpu_secs = cpu_ticks / proc_stathz;

    return snpack(symon_buf, maxlen, st->arg, MT_PROC,
                  n,
                  cpu_uticks, cpu_sticks, cpu_iticks, cpu_secs, cpu_pcti,
                  mem_procsize, mem_rss );
}
