/*
 * Copyright (c) 2001-2024 Willem Dijkstra
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

#ifndef _SYMON_LIB_NET_H
#define _SYMON_LIB_NET_H

#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/types.h>

#define SYMUX_PORT  "2100"      /* default symux port */
extern char res_host[];
extern char res_service[];
extern struct sockaddr_storage res_addr;

__BEGIN_DECLS
int cmpsock_addr(struct sockaddr *, struct sockaddr *);
int get_numeric_name(struct sockaddr_storage *);
int getaddr(char *, char *, int, int);
int getip(char *, int);
int lookup(char *);
void cpysock(struct sockaddr *, struct sockaddr_storage *);
void get_sockaddr(struct sockaddr_storage *, int, int, int, char*, char *);
void get_mux_sockaddr(struct mux *, int);
int get_source_sockaddr(struct source *, int);
__END_DECLS

#endif                          /* _SYMON_LIB_NET_H */
