/* $Id: symuxnet.h,v 1.4 2002/08/31 16:09:55 dijkstra Exp $ */

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

#ifndef _MONMUX_MUXNET_H
#define _MONMUX_MUXNET_H

#include "data.h"

/* Rewrite an ipadress uint32_t as 4 comma seperated bytes */
#define IPAS4BYTES(x) \
        ((x) >> 24), ((x) >> 16) & 0xff, ((x) >> 8) & 0xff, (x) & 0xff

/* prototypes */
__BEGIN_DECLS
int  acceptconnection(int);
int  getmonsocket(struct mux *);
int  getclientsocket(struct mux *);
void waitfortraffic(struct mux *, struct sourcelist *, 
		    struct source **, struct monpacket *);
int  recvmonpacket(struct mux *, struct sourcelist *, 
		   struct source **, struct monpacket *);
int  check_crc_packet(struct monpacket *);
__END_DECLS
#endif /*_MONMUX_MUXNET_H*/

