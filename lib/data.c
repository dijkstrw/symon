/* $Id: data.c,v 1.8 2002/04/04 20:49:58 dijkstra Exp $ */

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

/* A host carrying a 'mon' is considered a 'source' of information. A single
 * data 'stream' of information has a particular type: <cpu|mem|if|io>. A
 * source can provide multiple 'streams' simultaniously.A source spools
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
#include "xmalloc.h"

__BEGIN_DECLS
int            bytelenvar(char);
int            checklen(int, int, int);
struct stream *create_stream(int, char *);
char          *formatstrvar(char);
char          *rrdstrvar(char);
int            strlenvar(char);
__END_DECLS

/* Stream formats
 * 
 * Format specifications are strings of characters:
 *
 * L = u_int64 
 * l = u_int32
 * c = 3.2f <= u_int14 <= u_int16  (used in percentages)
 */
struct {
    char type;
    char *rrdformat;
    char *strformat;
    int  strlen;
    int  bytelen;
    u_int64_t max;
} streamvar[] = {
    {'L', ":%qu",    " %20qu",   22, sizeof(u_int64_t), (u_int64_t) 0xffffffffffffffff},
    {'l', ":%lu",    " %10lu",   12, sizeof(u_int32_t), (u_int64_t) 0xffffffff},
    {'c', ":%3.2f", "  %3.2f",    7, sizeof(u_int16_t), (u_int64_t) 100},
    {'\0', NULL,         NULL,    0,                 0,             0}
};
/* streams of <type> have the packedstream <form> */
struct {
    int type;
    char *form;
} streamform[] = {
    {MT_IO,  "LLL"},
    {MT_CPU, "ccccc"},
    {MT_MEM, "lllll"},
    {MT_IF,  "llllllllll"},
    {MT_EOT, ""}
};

struct {
    int type;
    int token;
} streamtoken[] = {
    {MT_IO,  LXT_IO},
    {MT_CPU, LXT_CPU},
    {MT_MEM, LXT_MEM},
    {MT_IF,  LXT_IF},
    {MT_EOT, LXT_BADTOKEN}
};

/* Convert lexical entities to stream entities */
const int 
token2type(const int token)
{
    int i;

    for (i=0; streamtoken[i].type < MT_EOT; i++) 
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

    for (i=0; streamtoken[i].type < MT_EOT; i++) 
	if (streamtoken[i].type == streamtype) 
	    return parse_opcode(streamtoken[i].token);

    fatal("%s:%d: internal error: type (%d) could not be translated into ascii representation", 
	  __FILE__, __LINE__, streamtype);

    /* NOT REACHED */
    return 0;
}
/* Return the maximum lenght of the ascii representation of streamvar <var> */
int 
strlenvar(char var)
{
    int i;

    for (i=0; streamvar[i].type > '\0'; i++)
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

    for (i=0; streamvar[i].type > '\0'; i++)
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

    for (i=0; streamvar[i].type > '\0'; i++)
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

    for (i=0; streamvar[i].type > '\0'; i++)
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
    } else {
	warning("buffer overflow: max=%d, current=%d, extra=%d",
		maxlen, current, extra);
	return 1;
    }
}
/* 
 * Pack multiple arguments of a MT_TYPE into a network order bytestream.
 * snpack returns the number of bytes actually stored.  
 */
int 
snpack(char *buf, int maxlen, char *id, int type, ...)
{
    va_list ap;
    u_int16_t c;
    u_int32_t l;
    u_int64_t q;
    int i = 0;
    int offset = 0;

    if (type > MT_EOT) {
	warning("stream type (%d) out of range", type);
	return 0;
    }

    if ( maxlen < 2 ) {
	fatal("%s:%d: maxlen too small", __FILE__, __LINE__);
    } else {
	buf[offset++] = type & 0xff;
    }

    if (id) {
	if ((strlen(id) + 1) >= maxlen) {
	    return 0;
	} else {
	    strncpy(&buf[offset], id, maxlen-1);
	    offset += strlen(id);
	}
    }
    buf[offset++] = '\0';
	        	    
    va_start(ap, type);
    while (streamform[type].form[i] != '\0'){
	/* check for buffer overflow */
	if (checklen(maxlen, offset, bytelenvar(streamform[type].form[i])))
	    return offset;
		     
	switch (streamform[type].form[i]) {
	case 'c':
	    c = va_arg(ap, u_int16_t);
	    c = htons(c);
	    bcopy(&c, buf + offset, sizeof(u_int16_t));
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
sunpack(char *buf, struct packedstream *ps)
{
    char *in, *out;
    int i=0;
    int type;
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
	ps->args[sizeof(ps->args)-1]='\0';
	in += strlen(ps->args);
    } else {
	ps->args[0] = '\0';
    }

    in++;

    out = (char *)(&ps->data);

    while (streamform[type].form[i] != '\0') {
	switch (streamform[type].form[i]) {
	case 'c':
	    c = *((u_int16_t *)in);
	    c = ntohs(c);
	    bcopy(&c, (void *)out, sizeof(u_int16_t));
	    in  += sizeof(u_int16_t);
	    out += sizeof(u_int16_t);
	    break;

	case 'l': 
	    l = *((u_int32_t *)in);
	    l = ntohl(l);
	    bcopy(&l, (void *)out, sizeof(u_int32_t));
	    in  += sizeof(u_int32_t);
	    out += sizeof(u_int32_t);
	    break;

	case 'L': 
	    q = *((u_int64_t *)in);
	    q = ntohq(q);
	    bcopy(&q, (void *)out, sizeof(u_int64_t));
	    in  += sizeof(u_int64_t);
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
ps2strn(struct packedstream *ps, char *buf, const int maxlen, int pretty)
{
    float c;
    u_int64_t q;
    u_int32_t l;
    int i=0;
    char *formatstr;
    char *in, *out;
    char vartype;
    
    in = (char *)(&ps->data);
    out = (char *)buf;

    while ((vartype = streamform[ps->type].form[i]) != '\0') {
	/* check buffer overflow */
	if (checklen(maxlen, (out-buf), strlenvar(vartype)))
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
	case 'c':
	    c = (*((u_int16_t *)in) / 10);
	    snprintf(out, strlenvar(vartype), formatstr, c); 
	    in  += sizeof(u_int16_t);
	    break;

	case 'l': 
	    l = *((u_int32_t *)in);
	    snprintf(out, strlenvar(vartype), formatstr, l); 
	    in  += sizeof(u_int32_t);
	    break;

	case 'L': 
	    q = *((u_int64_t *)in);
	    snprintf(out, strlenvar(vartype), formatstr, q); 
	    in  += sizeof(u_int64_t);
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

    p = (struct stream *)xmalloc(sizeof(struct stream));
    bzero(p, sizeof(struct stream));
    p->type = type;

    if (args != NULL)
	p->args = xstrdup(args);
    
    return p;
}
/* Find the stream handle in a source */
struct stream *
find_source_stream(struct source *source, int type, char *args) 
{
    struct stream *p;

    if (source == NULL)
	return NULL;

    SLIST_FOREACH(p, &source->sl, streams) {
	if ((p->type == type) 
	    && (((void *)args != (void *)p != NULL) 
		&& strncmp(args, p->args, _POSIX2_LINE_MAX) == 0))
	    return p;
    }

    return NULL;
}
/* Add a stream to a source */
struct stream *
add_source_stream(struct source *source, int type, char *args) 
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
find_mux_stream(struct mux *mux, int type, char *args) 
{
    struct stream *p;

    if (mux == NULL)
	return NULL;

    SLIST_FOREACH(p, &mux->sl, streams) {
	if ((p->type == type) 
	    && (((void *)args != (void *)p != NULL) 
		&& strncmp(args, p->args, _POSIX2_LINE_MAX) == 0))
	    return p;
    }

    return NULL;
}
/* Add a stream to a mux */
struct stream *
add_mux_stream(struct mux *mux, int type, char *args) 
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
find_source(struct sourcelist *sol, char *name) 
{
    struct source *p;

    if (sol == NULL || SLIST_EMPTY(sol))
	return NULL;

    SLIST_FOREACH(p, sol, sources) {
	if (((void *)name != (void *)p != NULL) 
	    && strncmp(name, p->name, _POSIX2_LINE_MAX) == 0)
	    return p;
    }

    return NULL;
}
/* Find a source by ip in a sourcelist */
struct source *
find_source_ip(struct sourcelist *sol, u_int32_t ip) 
{
    struct source *p;

    if (sol == NULL || SLIST_EMPTY(sol))
	return NULL;

    SLIST_FOREACH(p, sol, sources) {
	if (p->ip == ip)
	    return p;
    }

    return NULL;
}
/* Add a source with to a sourcelist */
struct source *
add_source(struct sourcelist *sol, char *name)
{
    struct source* p;

    if (sol == NULL)
	return NULL;

    if (find_source(sol, name) != NULL)
	return NULL;

    p = (struct source *)xmalloc(sizeof(struct source));
    bzero(p, sizeof(struct source));
    p->name = xstrdup(name);
    SLIST_INSERT_HEAD(sol, p, sources);

    return p;
}
/* Find a mux by name in a muxlist */
struct mux *
find_mux(struct muxlist *mul, char *name) 
{
    struct mux *p;

    if (mul == NULL || SLIST_EMPTY(mul))
	return NULL;

    SLIST_FOREACH(p, mul, muxes) {
	if (((void *)name != (void *)p != NULL) 
	    && strncmp(name, p->name, _POSIX2_LINE_MAX) == 0)
	    return p;
    }

    return NULL;
}
/* Add a mux to a muxlist */
struct mux *
add_mux(struct muxlist *mul, char *name)
{
    struct mux* p;

    if (mul == NULL)
	return NULL;

    if (find_mux(mul, name) != NULL)
	return NULL;

    p = (struct mux *)xmalloc(sizeof(struct mux));
    bzero(p, sizeof(struct mux));
    p->name = xstrdup(name);
    SLIST_INSERT_HEAD(mul, p, muxes);

    return p;
}
/* Rename a mux */
struct mux *
rename_mux(struct muxlist *mul, struct mux *mux, char *name)
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
free_muxlist(struct muxlist *mul) 
{
    struct mux *p, *np;

    if (mul == NULL || SLIST_EMPTY(mul))
	return;

    p = SLIST_FIRST(mul);

    while ( p ) {
	np = SLIST_NEXT(p, muxes);

	if (p->name != NULL) xfree(p->name);
	close(p->socket);
	free_streamlist(&p->sl);
	xfree(p);

	p = np;
    }
}
void 
free_streamlist(struct streamlist *sl) 
{
    struct stream *p, *np;

    if (sl == NULL || SLIST_EMPTY(sl))
	return;
    
    p = SLIST_FIRST(sl);

    while ( p ) {
	np = SLIST_NEXT(p, streams);

	if (p->args != NULL) xfree(p->args);
	if (p->file != NULL) xfree(p->file);
	xfree(p);

	p = np;
    }
}
void 
free_sourcelist(struct sourcelist *sol) 
{
    struct source *p, *np;

    if (sol == NULL || SLIST_EMPTY(sol))
	return;
    
    p = SLIST_FIRST(sol);

    while ( p ) {
	np = SLIST_NEXT(p, sources);

	if (p->name != NULL) xfree(p->name);
	free_streamlist(&p->sl);
	xfree(p);

	p = np;
    }
}
    
