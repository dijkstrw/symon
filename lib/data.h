/* $Id: data.h,v 1.33 2008/01/30 12:06:49 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2008 Willem Dijkstra
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
 * A host carrying a 'symon' is considered a 'source' of information. A single
 * data 'stream' of information has a particular type: <cpu|mem|if|io>. A
 * source can provide multiple 'streams' simultaneously. A source spools
 * information towards a 'mux'. A 'stream' that has been converted to network
 * representation is called a 'packedstream'.
 */
#ifndef _SYMON_LIB_DATA_H
#define _SYMON_LIB_DATA_H

#include "platform.h"

#include <stdarg.h>
#include <netinet/in.h>
#include <limits.h>

#include "lex.h"
#include "sylimits.h"

/* Polynominal to use for CRC generation */
#define SYMON_CRCPOLY  0x04c11db7

#ifndef ntohq
#if BYTE_ORDER == BIG_ENDIAN
#define htonq(n) (n)
#define ntohq(n) (n)
#else
static inline u_int64_t
       htonq(u_int64_t v)
{
    return (u_int64_t) htonl(v) << 32 | htonl(v >> 32);
}
static inline u_int64_t
       ntohq(u_int64_t v)
{
    return (u_int64_t) ntohl(v) << 32 | ntohl(v >> 32);
}
#endif /* BYTE_ORDER */
#endif /* ntohq */

/* Symon packet version
 * version 1 and 2:
 * symon_version:timestamp:length:crc:n*packedstream
 * packedstream = type:arg[<SYMON_PS_ARGLENVx]:data
 */
#define SYMON_PACKET_VER  2
#define SYMON_UNKMUX   "<unknown mux>"  /* mux nodes without host addr */

/* Sending structures over the network is dangerous as the compiler might have
 * added extra padding between items. symonpacketheader below is therefore also
 * marshalled and demarshalled via snpack and sunpack. The actual values are
 * copied out of memory into this structure one by one.
 */
struct symonpacketheader {
    u_int64_t timestamp;
    u_int32_t crc;
    u_int16_t length;
    u_int8_t symon_version;
    u_int8_t reserved;
};

struct symonpacket {
    struct symonpacketheader header;
    u_int32_t offset;
    u_int32_t size;
    char *data;
};
/* The difference between a stream and a packed stream:
 * - A stream ties stream information to a file.
 * - A packed stream is the measured data itself
 *
 * A stream is meta data describing properties, a packed stream is the data itself.
 */
struct stream {
    int type;
    char *arg;
    char *file;
    SLIST_ENTRY(stream) streams;
    union stream_parg parg;
};
SLIST_HEAD(streamlist, stream);

struct source {
    char *addr;
    struct sockaddr_storage sockaddr;
    struct streamlist sl;
    SLIST_ENTRY(source) sources;
};
SLIST_HEAD(sourcelist, source);

struct mux {
    char *name;
    char *addr;
    char *port;
    char *localaddr;
    struct sourcelist sol;
    int clientsocket;           /* symux; incoming tcp connections */
    int symonsocket[AF_MAX];    /* symux; incoming symon data */
    int symuxsocket;            /* symon; outgoing data to mux */
    struct symonpacket packet;
    struct sockaddr_storage sockaddr;
    struct streamlist sl;
    u_int32_t senderr;
    SLIST_ENTRY(mux) muxes;
};
SLIST_HEAD(muxlist, mux);

/* ps2str types */
#define PS2STR_PRETTY 0
#define PS2STR_RRD    1

/* Stream types
 *
 * Add new items at the bottom to preserve compatibility with older symon
 * instances
 */
#define MT_IO1    0
#define MT_CPU    1
#define MT_MEM1   2
#define MT_IF1    3
#define MT_PF     4
#define MT_DEBUG  5
#define MT_PROC   6
#define MT_MBUF   7
#define MT_SENSOR 8
#define MT_IO2    9
#define MT_PFQ    10
#define MT_DF     11
#define MT_MEM2   12
#define MT_IF2    13
#define MT_CPUIOW 14
#define MT_TEST   15
#define MT_EOT    16

/*
 * Unpacking of incoming packets is done via a packedstream structure. This
 * structure defines the maximum amount of data that can be contained in a
 * single network representation of a stream.
 */
struct packedstream {
    int type;
    int padding;
    char arg[SYMON_PS_ARGLENV2]; /* V2 > V1 */
    union {
        struct symonpacketheader header;
        struct {
            u_int64_t mtotal_transfers;
            u_int64_t mtotal_seeks;
            u_int64_t mtotal_bytes;
        }      ps_io1;
        struct {
            u_int16_t muser;
            u_int16_t mnice;
            u_int16_t msystem;
            u_int16_t minterrupt;
            u_int16_t midle;
        }      ps_cpu;
        struct {
            u_int32_t mreal_active;
            u_int32_t mreal_total;
            u_int32_t mfree;
            u_int32_t mswap_used;
            u_int32_t mswap_total;
        }      ps_mem1;
        struct {
            u_int32_t mipackets;
            u_int32_t mopackets;
            u_int32_t mibytes;
            u_int32_t mobytes;
            u_int32_t mimcasts;
            u_int32_t momcasts;
            u_int32_t mierrors;
            u_int32_t moerrors;
            u_int32_t mcolls;
            u_int32_t mdrops;
        }      ps_if1;
        struct {
            u_int64_t bytes_v4_in;
            u_int64_t bytes_v4_out;
            u_int64_t bytes_v6_in;
            u_int64_t bytes_v6_out;
            u_int64_t packets_v4_in_pass;
            u_int64_t packets_v4_in_drop;
            u_int64_t packets_v4_out_pass;
            u_int64_t packets_v4_out_drop;
            u_int64_t packets_v6_in_pass;
            u_int64_t packets_v6_in_drop;
            u_int64_t packets_v6_out_pass;
            u_int64_t packets_v6_out_drop;
            u_int64_t states_entries;
            u_int64_t states_searches;
            u_int64_t states_inserts;
            u_int64_t states_removals;
            u_int64_t counters_match;
            u_int64_t counters_badoffset;
            u_int64_t counters_fragment;
            u_int64_t counters_short;
            u_int64_t counters_normalize;
            u_int64_t counters_memory;
        }      ps_pf;
        struct {
            u_int32_t debug0;
            u_int32_t debug1;
            u_int32_t debug2;
            u_int32_t debug3;
            u_int32_t debug4;
            u_int32_t debug5;
            u_int32_t debug6;
            u_int32_t debug7;
            u_int32_t debug8;
            u_int32_t debug9;
            u_int32_t debug10;
            u_int32_t debug11;
            u_int32_t debug12;
            u_int32_t debug13;
            u_int32_t debug14;
            u_int32_t debug15;
            u_int32_t debug16;
            u_int32_t debug17;
            u_int32_t debug18;
            u_int32_t debug19;
        }      ps_debug;
        struct {
            u_int32_t totmbufs;
            u_int32_t mt_data;
            u_int32_t mt_oobdata;
            u_int32_t mt_control;
            u_int32_t mt_header;
            u_int32_t mt_ftable;
            u_int32_t mt_soname;
            u_int32_t mt_soopts;
            u_int32_t pgused;
            u_int32_t pgtotal;
            u_int32_t totmem;
            u_int32_t totpct;
            u_int32_t m_drops;
            u_int32_t m_wait;
            u_int32_t m_drain;
        }      ps_mbuf;
        struct {
            int64_t value;
        }      ps_sensor;
        struct {
            u_int64_t mtotal_rtransfers;
            u_int64_t mtotal_wtransfers;
            u_int64_t mtotal_seeks2;
            u_int64_t mtotal_rbytes;
            u_int64_t mtotal_wbytes;
        }      ps_io2;
        struct {
            u_int64_t sent_bytes;
            u_int64_t sent_packets;
            u_int64_t drop_bytes;
            u_int64_t drop_packets;
        }      ps_pfq;
        struct {
            u_int64_t L[4];
            int64_t D[4];
            u_int32_t l[4];
            u_int16_t s[4];
            u_int16_t c[4];
            u_int8_t b[4];
        }      ps_test;
        struct {
            u_int64_t blocks;
            u_int64_t bfree;
            u_int64_t bavail;
            u_int64_t files;
            u_int64_t ffree;
            u_int64_t syncwrites;
            u_int64_t asyncwrites;
        }      ps_df;
        struct {
            u_int64_t mreal_active;
            u_int64_t mreal_total;
            u_int64_t mfree;
            u_int64_t mswap_used;
            u_int64_t mswap_total;
        }      ps_mem2;
        struct {
            u_int64_t mipackets;
            u_int64_t mopackets;
            u_int64_t mibytes;
            u_int64_t mobytes;
            u_int64_t mimcasts;
            u_int64_t momcasts;
            u_int64_t mierrors;
            u_int64_t moerrors;
            u_int64_t mcolls;
            u_int64_t mdrops;
        }      ps_if2;
        struct {
            u_int16_t muser;
            u_int16_t mnice;
            u_int16_t msystem;
            u_int16_t minterrupt;
            u_int16_t midle;
            u_int16_t miowait;
        }      ps_cpuiow;
    }     data;
};

/* prototypes */
__BEGIN_DECLS
char *type2str(const int);
int bytelen_sourcelist(struct sourcelist *);
int bytelen_streamlist(struct streamlist *);
int getheader(char *, struct symonpacketheader *);
int ps2strn(struct packedstream *, char *, int, int);
int setheader(char *, struct symonpacketheader *);
int snpack(char *, int, char *, int, ...);
int snpack1(char *, int, char *, int, ...);
int snpack2(char *, int, char *, int, ...);
int snpackx(size_t, char *, int, char *, int, va_list);
int strlen_sourcelist(struct sourcelist *);
int strlentype(int);
int sunpack1(char *, struct packedstream *);
int sunpack2(char *, struct packedstream *);
int sunpackx(size_t, char *, struct packedstream *);
int token2type(const int);
struct mux *add_mux(struct muxlist *, char *);
struct mux *find_mux(struct muxlist *, char *);
struct mux *rename_mux(struct muxlist *, struct mux *, char *);
struct source *add_source(struct sourcelist *, char *);
struct source *find_source(struct sourcelist *, char *);
struct source *find_source_sockaddr(struct sourcelist *, struct sockaddr *);
struct stream *add_mux_stream(struct mux *, int, char *);
struct stream *add_source_stream(struct source *, int, char *);
struct stream *find_mux_stream(struct mux *, int, char *);
struct stream *find_source_stream(struct source *, int, char *);
u_int32_t crc32(const void *, unsigned int);
void free_muxlist(struct muxlist *);
void free_sourcelist(struct sourcelist *);
void free_streamlist(struct streamlist *);
void init_crc32();
void init_symon_packet(struct mux *);
void init_symux_packet(struct mux *);
__END_DECLS
#endif                          /* _SYMON_LIB_DATA_H */
