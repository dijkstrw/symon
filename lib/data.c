/* $Id: data.c,v 1.18 2002/12/15 14:27:43 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2002 Willem Dijkstra
 * All rights reserved.
 *
 * The crc routine is from Rob Warnock <rpw3@sgi.com>, from the
 * comp.compression FAQ.
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
 * */

/* Terminology:
 *
 * A host carrying a 'symon' is considered a 'source' of information. A single
 * data 'stream' of information has a particular type: <cpu|mem|if|io>. A
 * source can provide multiple 'streams' simultaniously. A source spools
 * information towards a 'mux'. A 'stream' that has been converted to network
 * representation is called a 'packedstream'.
 */
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "data.h"
#include "error.h"
#include "lex.h"
#include "net.h"
#include "xmalloc.h"

__BEGIN_DECLS
int bytelenvar(char);
int checklen(int, int, int);
struct stream *create_stream(int, char *);
char *formatstrvar(char);
char *rrdstrvar(char);
int strlenvar(char);
__END_DECLS

/* Stream formats
 *
 * Format specifications are strings of characters:
 *
 * L = u_int64
 * l = u_int32
 * s = u_int16
 * c = 3.2f <= u_int14 <= u_int16  (used in percentages)
 * b = u_int8
 */
struct {
    char type;
    char *rrdformat;
    char *strformat;
    int strlen;
    int bytelen;
    u_int64_t max;
}      streamvar[] = {
    { 'L', ":%llu", " %20llu", 22, sizeof(u_int64_t), (u_int64_t) 0xffffffffffffffff },
    { 'l', ":%lu", " %10lu", 12, sizeof(u_int32_t), (u_int64_t) 0xffffffff },
    { 's', ":%u", " %5u", 7, sizeof(u_int16_t), (u_int64_t) 0xffff },
    { 'c', ":%3.2f", " %3.2f", 8, sizeof(u_int16_t), (u_int64_t) 100 },
    { 'b', ":%3u", " %3u", 5, sizeof(u_int8_t), (u_int64_t) 255 },
    { '\0', NULL, NULL, 0, 0, 0 }
};
/* streams of <type> have the packedstream <form> */
struct {
    int type;
    char *form;
}      streamform[] = {
    { MT_IO, "LLL" },
    { MT_CPU, "ccccc" },
    { MT_MEM, "lllll" },
    { MT_IF, "llllllllll" },
    { MT_PF, "LLLLLLLLLLLLLLLLLLLLLL" },
    { MT_DEBUG, "llllllllllllllllllll" },
    { MT_PROC, "lLLLlll" },
    { MT_EOT, "" }
};

struct {
    int type;
    int token;
}      streamtoken[] = {
    { MT_IO, LXT_IO },
    { MT_CPU, LXT_CPU },
    { MT_MEM, LXT_MEM },
    { MT_IF, LXT_IF },
    { MT_PF, LXT_PF },
    { MT_DEBUG, LXT_DEBUG },
    { MT_PROC, LXT_PROC },
    { MT_EOT, LXT_BADTOKEN }
};
/* parallel crc32 table */
u_int32_t
crc32_table[256];

/* Convert lexical entities to stream entities */
const int 
token2type(const int token)
{
    int i;

    for (i = 0; streamtoken[i].type < MT_EOT; i++)
	if (streamtoken[i].token == token)
	    return streamtoken[i].type;

    fatal("%s:%d: internal error: token (%d) could not be translated into a stream type",
	  __FILE__, __LINE__, token);

    /* NOT REACHED */
    return 0;
}
/* Convert stream entities to their ascii representation */
const char *
type2str(const int streamtype)
{
    int i;

    for (i = 0; streamtoken[i].type < MT_EOT; i++)
	if (streamtoken[i].type == streamtype)
	    return parse_opcode(streamtoken[i].token);

    fatal("%s:%d: internal error: type (%d) could not be translated into ascii representation",
	  __FILE__, __LINE__, streamtype);

    /* NOT REACHED */
    return 0;
}
/* Return the maximum lenght of the ascii representation of type <type> */
int 
strlentype(int type)
{
    int i = 0;
    int sum = 0;

    while (streamform[type].form[i])
	sum += strlenvar(streamform[type].form[i++]);

    return sum;
}
/* Return the maximum lenght of the ascii representation of streamvar <var> */
int 
strlenvar(char var)
{
    int i;

    for (i = 0; streamvar[i].type > '\0'; i++)
	if (streamvar[i].type == var)
	    return streamvar[i].strlen;

    fatal("%s:%d: internal error: type spefication for stream var '%c' not found",
	  __FILE__, __LINE__, var);

    /* NOT REACHED */
    return 0;
}
/* Return the maximum lenght of the network representation of streamvar <var> */
int 
bytelenvar(char var)
{
    int i;

    for (i = 0; streamvar[i].type > '\0'; i++)
	if (streamvar[i].type == var)
	    return streamvar[i].bytelen;

    fatal("%s:%d: internal error: type spefication for stream var '%c' not found",
	  __FILE__, __LINE__, var);

    /* NOT REACHED */
    return 0;
}
/* Return the ascii format string for streamvar <var> */
char *
formatstrvar(char var)
{
    int i;

    for (i = 0; streamvar[i].type > '\0'; i++)
	if (streamvar[i].type == var)
	    return streamvar[i].strformat;

    fatal("%s:%d: internal error: type spefication for stream var '%c' not found",
	  __FILE__, __LINE__, var);

    /* NOT REACHED */
    return "";
}
/* Return the rrd format string for streamvar <var> */
char *
rrdstrvar(char var)
{
    int i;

    for (i = 0; streamvar[i].type > '\0'; i++)
	if (streamvar[i].type == var)
	    return streamvar[i].rrdformat;

    fatal("internal error: type spefication for stream var '%c' not found", var);

    /* NOT REACHED */
    return "";
}
/* Check whether <extra> more bytes fit in <maxlen> when we are already at <start> */
int 
checklen(int maxlen, int current, int extra)
{
    if ((current + extra) < maxlen) {
	return 0;
    }
    else {
	warning("buffer overflow: max=%d, current=%d, extra=%d",
		maxlen, current, extra);
	return 1;
    }
}
int 
setheader(char *buf, struct symonpacketheader *hph)
{
    struct symonpacketheader nph;
    char *p;

    nph.timestamp = htonq(hph->timestamp);
    nph.crc = htonl(hph->crc);
    nph.length = htons(hph->length);
    nph.symon_version = hph->symon_version;

    p = buf;

    bcopy(&nph.crc, p, sizeof(u_int32_t));
    p += sizeof(u_int32_t);
    bcopy(&nph.timestamp, p, sizeof(u_int64_t));
    p += sizeof(u_int64_t);
    bcopy(&nph.length, p, sizeof(u_int16_t));
    p += sizeof(u_int16_t);
    bcopy(&nph.symon_version, p, sizeof(u_int8_t));
    p += sizeof(u_int8_t);

    return (p - buf);
}
int 
getheader(char *buf, struct symonpacketheader *hph)
{
    char *p;

    p = buf;

    bcopy(p, &hph->crc, sizeof(u_int32_t));
    p += sizeof(u_int32_t);
    bcopy(p, &hph->timestamp, sizeof(u_int64_t));
    p += sizeof(u_int64_t);
    bcopy(p, &hph->length, sizeof(u_int16_t));
    p += sizeof(u_int16_t);
    bcopy(p, &hph->symon_version, sizeof(u_int8_t));
    p += sizeof(u_int8_t);

    hph->timestamp = ntohq(hph->timestamp);
    hph->crc = ntohl(hph->crc);
    hph->length = ntohs(hph->length);

    return (p - buf);
}
/*
 * Pack multiple arguments of a MT_TYPE into a network order bytestream.
 * snpack returns the number of bytes actually stored.
 */
int 
snpack(char *buf, int maxlen, char *id, int type,...)
{
    va_list ap;
    u_int16_t b;
    u_int16_t s;
    u_int16_t c;
    u_int32_t l;
    u_int64_t q;
    int i = 0;
    int offset = 0;

    if (type > MT_EOT) {
	warning("stream type (%d) out of range", type);
	return 0;
    }

    if (maxlen < 2) {
	fatal("%s:%d: maxlen too small", __FILE__, __LINE__);
    }
    else {
	buf[offset++] = type & 0xff;
    }

    if (id) {
	if ((strlen(id) + 1) >= maxlen) {
	    return 0;
	}
	else {
	    strncpy(&buf[offset], id, maxlen - 1);
	    offset += strlen(id);
	}
    }
    buf[offset++] = '\0';

    va_start(ap, type);
    while (streamform[type].form[i] != '\0') {
	/* check for buffer overflow */
	if (checklen(maxlen, offset, bytelenvar(streamform[type].form[i])))
	    return offset;

	/*
	 * all values smaller than 32 bytes are transferred using ints on the
	 * stack. This is to ensure that we get the correct value, if the
	 * compiler decided to upgrade our short to a 32bit int. -- cheers
	 * dhartmei@
	 */
	switch (streamform[type].form[i]) {
	case 'b':
	    b = va_arg(ap, int);
	    buf[offset++] = b;
	    break;

	case 'c':
	    c = va_arg(ap, int);
	    c = htons(c);
	    bcopy(&c, buf + offset, sizeof(u_int16_t));
	    offset += sizeof(u_int16_t);
	    break;

	case 's':
	    s = va_arg(ap, int);
	    s = htons(c);
	    bcopy(&s, buf + offset, sizeof(u_int16_t));
	    offset += sizeof(u_int16_t);
	    break;

	case 'l':
	    l = va_arg(ap, u_int32_t);
	    l = htonl(l);
	    bcopy(&l, buf + offset, sizeof(u_int32_t));
	    offset += sizeof(u_int32_t);
	    break;

	case 'L':
	    q = va_arg(ap, u_int64_t);
	    q = htonq(q);
	    bcopy(&q, buf + offset, sizeof(u_int64_t));
	    offset += sizeof(u_int64_t);
	    break;

	default:
	    warning("unknown stream format identifier");
	    return 0;
	}
	i++;
    }
    va_end(ap);

    return offset;
}
/*
 * Unpack a packedstream in buf into a struct packetstream. Returns the number
 * of bytes actually read.
 *
 * Note that this function does "automatic" bounds checking; it uses a
 * description of the packedstream (streamform) to parse the actual bytes. This
 * description corresponds to the amount of bytes that will fit inside the
 * packedstream structure.  */
int 
sunpack(char *buf, struct packedstream * ps)
{
    char *in, *out;
    int i = 0;
    int type;
    u_int16_t s;
    u_int16_t c;
    u_int32_t l;
    u_int64_t q;

    bzero(ps, sizeof(struct packedstream));

    in = buf;

    if ((*in) > MT_EOT) {
	warning("unpack failure: stream type (%d) out of range", (*in));
	return -1;
    }

    type = ps->type = (*in);
    in++;
    if ((*in) != '\0') {
	strncpy(ps->args, in, sizeof(ps->args));
	ps->args[sizeof(ps->args) - 1] = '\0';
	in += strlen(ps->args);
    }
    else {
	ps->args[0] = '\0';
    }

    in++;

    out = (char *) (&ps->data);

    while (streamform[type].form[i] != '\0') {
	switch (streamform[type].form[i]) {
	case 'b':
	    bcopy((void *) in, (void *) out, sizeof(u_int8_t));
	    in++;
	    out++;
	    break;

	case 'c':
	    bcopy((void *) in, &c, sizeof(u_int16_t));
	    c = ntohs(c);
	    bcopy(&c, (void *) out, sizeof(u_int16_t));
	    in += sizeof(u_int16_t);
	    out += sizeof(u_int16_t);
	    break;

	case 's':
	    bcopy((void *) in, &s, sizeof(u_int16_t));
	    s = ntohs(s);
	    bcopy(&s, (void *) out, sizeof(u_int16_t));
	    in += sizeof(u_int16_t);
	    out += sizeof(u_int16_t);
	    break;

	case 'l':
	    bcopy((void *) in, &l, sizeof(u_int32_t));
	    l = ntohl(l);
	    bcopy(&l, (void *) out, sizeof(u_int32_t));
	    in += sizeof(u_int32_t);
	    out += sizeof(u_int32_t);
	    break;

	case 'L':
	    bcopy((void *) in, &q, sizeof(u_int64_t));
	    q = ntohq(q);
	    bcopy(&q, (void *) out, sizeof(u_int64_t));
	    in += sizeof(u_int64_t);
	    out += sizeof(u_int64_t);
	    break;

	default:
	    warning("unknown stream format identifier");
	    return 0;
	}
	i++;
    }
    return (in - buf);
}
/* Get the RRD or 'pretty' ascii representation of packedstream */
int 
ps2strn(struct packedstream * ps, char *buf, const int maxlen, int pretty)
{
    float f;
    u_int16_t b;
    u_int16_t s;
    u_int16_t c;
    u_int64_t q;
    u_int32_t l;
    int i = 0;
    char *formatstr;
    char *in, *out;
    char vartype;

    in = (char *) (&ps->data);
    out = (char *) buf;

    while ((vartype = streamform[ps->type].form[i]) != '\0') {
	/* check buffer overflow */
	if (checklen(maxlen, (out - buf), strlenvar(vartype)))
	    return 0;

	switch (pretty) {
	case PS2STR_PRETTY:
	    formatstr = formatstrvar(vartype);
	    break;
	case PS2STR_RRD:
	    formatstr = rrdstrvar(vartype);
	    break;
	default:
	    warning("%s:%d: unknown pretty identifier", __FILE__, __LINE__);
	    return 0;
	}

	switch (vartype) {
	case 'b':
	    bcopy(in, &b, sizeof(u_int8_t));
	    snprintf(out, strlenvar(vartype), formatstr, b);
	    in++;
	    break;

	case 'c':
	    bcopy(in, &c, sizeof(u_int16_t));
	    f = (float) c / 10.0;
	    snprintf(out, strlenvar(vartype), formatstr, f);
	    in += sizeof(u_int16_t);
	    break;

	case 's':
	    bcopy(in, &s, sizeof(u_int16_t));
	    snprintf(out, strlenvar(vartype), formatstr, s);
	    in += sizeof(u_int16_t);
	    break;

	case 'l':
	    bcopy(in, &l, sizeof(u_int32_t));
	    snprintf(out, strlenvar(vartype), formatstr, l);
	    in += sizeof(u_int32_t);
	    break;

	case 'L':
	    bcopy(in, &q, sizeof(u_int64_t));
	    snprintf(out, strlenvar(vartype), formatstr, q);
	    in += sizeof(u_int64_t);
	    break;

	default:
	    warning("Unknown stream format identifier");
	    return 0;
	}
	out += strlen(out);
	i++;
    }
    return (out - buf);
}
struct stream *
create_stream(int type, char *args)
{
    struct stream *p;

    if (type < 0 || type >= MT_EOT)
	fatal("%s:%d: internal error: stream type unknown", __FILE__, __LINE__);

    p = (struct stream *) xmalloc(sizeof(struct stream));
    bzero(p, sizeof(struct stream));
    p->type = type;

    if (args != NULL)
	p->args = xstrdup(args);

    return p;
}
/* Find the stream handle in a source */
struct stream *
find_source_stream(struct source * source, int type, char *args)
{
    struct stream *p;

    if (source == NULL)
	return NULL;

    SLIST_FOREACH(p, &source->sl, streams) {
	if ((p->type == type)
	    && (((void *) args != (void *) p != NULL)
		&& strncmp(args, p->args, _POSIX2_LINE_MAX) == 0))
	    return p;
    }

    return NULL;
}
/* Add a stream to a source */
struct stream *
add_source_stream(struct source * source, int type, char *args)
{
    struct stream *p;

    if (source == NULL)
	return NULL;

    if (find_source_stream(source, type, args) != NULL)
	return NULL;

    p = create_stream(type, args);

    SLIST_INSERT_HEAD(&source->sl, p, streams);

    return p;
}
/* Find a stream in a mux */
struct stream *
find_mux_stream(struct mux * mux, int type, char *args)
{
    struct stream *p;

    if (mux == NULL)
	return NULL;

    SLIST_FOREACH(p, &mux->sl, streams) {
	if ((p->type == type)
	    && (((void *) args != (void *) p != NULL)
		&& strncmp(args, p->args, _POSIX2_LINE_MAX) == 0))
	    return p;
    }

    return NULL;
}
/* Add a stream to a mux */
struct stream *
add_mux_stream(struct mux * mux, int type, char *args)
{
    struct stream *p;

    if (mux == NULL)
	return NULL;

    if (find_mux_stream(mux, type, args) != NULL)
	return NULL;

    p = create_stream(type, args);

    SLIST_INSERT_HEAD(&mux->sl, p, streams);

    return p;
}
/* Find a source by name in a sourcelist */
struct source *
find_source(struct sourcelist * sol, char *name)
{
    struct source *p;

    if (sol == NULL || SLIST_EMPTY(sol))
	return NULL;

    SLIST_FOREACH(p, sol, sources) {
	if (((void *) name != (void *) p != NULL)
	    && strncmp(name, p->addr, _POSIX2_LINE_MAX) == 0)
	    return p;
    }

    return NULL;
}
/* Find a source by ip in a sourcelist */
struct source *
find_source_sockaddr(struct sourcelist * sol, struct sockaddr * addr)
{
    struct source *p;

    if (sol == NULL || SLIST_EMPTY(sol))
	return NULL;

    SLIST_FOREACH(p, sol, sources) {
	if (cmpsock_addr((struct sockaddr *) & p->sockaddr, addr))
	    return p;
    }

    return NULL;
}
/* Add a source with to a sourcelist */
struct source *
add_source(struct sourcelist * sol, char *name)
{
    struct source *p;

    if (sol == NULL)
	return NULL;

    if (find_source(sol, name) != NULL)
	return NULL;

    p = (struct source *) xmalloc(sizeof(struct source));
    bzero(p, sizeof(struct source));
    p->addr = xstrdup(name);

    SLIST_INSERT_HEAD(sol, p, sources);

    return p;
}
/* Find a mux by name in a muxlist */
struct mux *
find_mux(struct muxlist * mul, char *name)
{
    struct mux *p;

    if (mul == NULL || SLIST_EMPTY(mul))
	return NULL;

    SLIST_FOREACH(p, mul, muxes) {
	if (((void *) name != (void *) p != NULL)
	    && strncmp(name, p->name, _POSIX2_LINE_MAX) == 0)
	    return p;
    }

    return NULL;
}
/* Add a mux to a muxlist */
struct mux *
add_mux(struct muxlist * mul, char *name)
{
    struct mux *p;

    if (mul == NULL)
	return NULL;

    if (find_mux(mul, name) != NULL)
	return NULL;

    p = (struct mux *) xmalloc(sizeof(struct mux));
    bzero(p, sizeof(struct mux));
    p->name = xstrdup(name);
    SLIST_INSERT_HEAD(mul, p, muxes);
    SLIST_INIT(&p->sol);

    return p;
}
/* Rename a mux */
struct mux *
rename_mux(struct muxlist * mul, struct mux * mux, char *name)
{
    if (mul == NULL || mux == NULL)
	return NULL;

    if (find_mux(mul, name) != NULL)
	return NULL;

    if (mux->name != NULL)
	xfree(mux->name);

    mux->name = xstrdup(name);

    return mux;
}
void 
free_muxlist(struct muxlist * mul)
{
    struct mux *p, *np;
    int i;

    if (mul == NULL || SLIST_EMPTY(mul))
	return;

    p = SLIST_FIRST(mul);

    while (p) {
	np = SLIST_NEXT(p, muxes);

	if (p->name != NULL)
	    xfree(p->name);
	if (p->addr != NULL)
	    xfree(p->addr);
	if (p->port != NULL)
	    xfree(p->port);

	close(p->clientsocket);
	close(p->symuxsocket);
	for (i = 0; i < AF_MAX; i++)
	    if (p->symonsocket[i])
		close(p->symonsocket[i]);

	free_streamlist(&p->sl);
	free_sourcelist(&p->sol);
	xfree(p);

	p = np;
    }
}
void 
free_streamlist(struct streamlist * sl)
{
    struct stream *p, *np;

    if (sl == NULL || SLIST_EMPTY(sl))
	return;

    p = SLIST_FIRST(sl);

    while (p) {
	np = SLIST_NEXT(p, streams);

	if (p->args != NULL)
	    xfree(p->args);
	if (p->file != NULL)
	    xfree(p->file);
	xfree(p);

	p = np;
    }
}
void 
free_sourcelist(struct sourcelist * sol)
{
    struct source *p, *np;

    if (sol == NULL || SLIST_EMPTY(sol))
	return;

    p = SLIST_FIRST(sol);

    while (p) {
	np = SLIST_NEXT(p, sources);

	if (p->addr != NULL)
	    xfree(p->addr);

	free_streamlist(&p->sl);
	xfree(p);

	p = np;
    }
}
/* Calculate maximum buffer space needed for a single symon hit */
int 
calculate_churnbuffer(struct sourcelist * sol)
{
    struct source *source;
    struct stream *stream;
    int prefixlen;
    int maxlen;
    int len;
    int n;

    /* determine length of a timestamp + ip as strings */
    prefixlen = (sizeof(time_t) * 3) + strlen(":") + 15 + strlen(":");

    len = n = 0;
    source = NULL;
    stream = NULL;
    maxlen = 0;
    /* determine maximum string size for a single source */
    SLIST_FOREACH(source, sol, sources) {
	len = prefixlen;
	SLIST_FOREACH(stream, &source->sl, streams) {
	    len += strlen(type2str(stream->type)) + strlen(":");
	    len += strlen(stream->args) + strlen(":");
	    len += strlentype(stream->type);
	    n++;
	}
	if (len > maxlen)
	    maxlen = len;
    }
    return maxlen;
}
/* Big endian CRC32 */
u_int32_t
crc32(const void *buf, unsigned int len)
{
    u_int8_t *p;
    u_int32_t crc;

    crc = 0xffffffff;
    for (p = (u_int8_t *) buf; len > 0; ++p, --len)
	crc = (crc << 8) ^ crc32_table[(crc >> 24) ^ *p];

    return ~crc;
}
/* Init table for CRC32 */
void 
init_crc32()
{
    unsigned int i, j;
    u_int32_t c;

    for (i = 0; i < 256; ++i) {
	c = i << 24;
	for (j = 8; j > 0; --j)
	    c = c & 0x80000000 ? (c << 1) ^ SYMON_CRCPOLY : (c << 1);
	crc32_table[i] = c;
    }
}
