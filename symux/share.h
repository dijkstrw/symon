/* $Id: share.h,v 1.7 2003/12/20 16:30:44 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2003 Willem Dijkstra
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

/* TODO:
 * Dynamically allocate buffer size
 * Check wether one buffer suffices, do some performance tests
 */

#ifndef _SYMUX_SHARE_H
#define _SYMUX_SHARE_H

#include "data.h"

/* Share contains all routines needed for the ipc between symuxes */
#define SEM_WAIT     0		/* wait semaphore */
#define SEM_READ     1		/* have read semaphore */
#define SEM_TOTAL    2

struct sharedregion {
    long seqnr;
    long reglen;		/* size of buffer */
    long ctlen;			/* amount of content in buffer, assert(<
				 * size) */
    long data;
};

/* prototypes */
__BEGIN_DECLS
void master_forbidread();
void master_permitread();
long shared_getlen();
long shared_getmaxlen();
long *shared_getmem();
void initshare();
void shared_setlen(long);
pid_t spawn_client(int);
__END_DECLS

#endif				/* _SYMUX_SHARE_H */
