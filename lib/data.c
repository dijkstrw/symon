/*
 * $Id: data.c,v 1.3 2002/03/29 15:16:34 dijkstra Exp $
 *
 * A host carrying a 'mon' is considered a 'source' of information. A single
 * data 'stream' of information has a particular type: <cpu|mem|if|io>. A
 * source can provide multiple 'streams' simultaniously.A source spools
 * information towards a 'mux'. A 'stream' that has been converted to network
 * representation is called a 'packedstream'.
 */
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "error.h"
#include "data.h"
#include "lex.h"
#include "xmalloc.h"

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
    char *strformat;
    int  strlen;
    int  bytelen;
    u_int64_t max;
} streamvar[] = {
    {'L', " %20qu",   21, sizeof(u_int64_t), (u_int64_t) 0xffffffffffffffff},
    {'l', " %10lu",   11, sizeof(u_int32_t), (u_int64_t) 0xffffffff},
    {'c', "  %3.2f",   6, sizeof(u_int16_t), (u_int64_t) 100},
    {'\0', NULL, 0, 0, 0}
};

struct {
    int type;
    char *form;
} streamform[] = {
/* io =     (u_int64) total nr of transfers : 
 *          (u_int64) total seeks : 
 *          (u_int64) total bytes transferred */
    {MT_IO,  "LLL"},
/* cpu =    (u_int16) user : (u_int16) nice : 
 *          (u_int16) system : (u_int16) interrupt : 
 *          (u_int16) idle
 * values = (3.2f) <= 100.00 */
    {MT_CPU, "ccccc"},
/* mem =    (u_int32) real active : 
 *          (u_int32) real total : 
 *          (u_int32) free : 
 *          (u_int32) swap used : 
 *          (u_int32) swap total */
    {MT_MEM, "lllll"},
/* ifstat 
 * if =     (u_int32) ipackets : 
 *          (u_int32) opackets : 
 *          (u_int32) ibytes : 
 *          (u_int32) obytes : 
 *          (u_int32) imcasts : 
 *          (u_int32) omcasts : 
 *          (u_int32) ierrors : 
 *          (u_int32) oerrors : 
 *          (u_int32) colls : 
 *          (u_int32) drops */
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

int token2type(token)
    int token;
{
    int i;

    for (i=0; streamtoken[i].type < MT_EOT; i++) 
	if (streamtoken[i].token == token) 
	    return streamtoken[i].type;

    fatal("internal error: Token (%d) could not be translated into a stream type.\n", 
	  token);

    /* NOT REACHED */
    return 0;
}
/* strlenvar(var)
 * return the maximum lenght that a streamvar of type var can have
 */
int strlenvar(var)
    char var;
{
    int i;

    for (i=0; streamvar[i].type > '\0'; i++)
	if (streamvar[i].type == var)
	    return streamvar[i].strlen;
    
    fatal("internal error: Type spefication for stream var '%c' not found", var);
    
    /* NOT REACHED */
    return 0;
}
/* bytelenvar(var)
 * return the maximum lenght that a streamvar of type var can have
 */
int bytelenvar(var)
    char var;
{
    int i;

    for (i=0; streamvar[i].type > '\0'; i++)
	if (streamvar[i].type == var)
	    return streamvar[i].bytelen;
    
    fatal("internal error: Type spefication for stream var '%c' not found", var);
    
    /* NOT REACHED */
    return 0;
}
/* formatstrvar(var)
 * return the maximum lenght that a streamvar of type var can have
 */
char *formatstrvar(var)
    char var;
{
    int i;

    for (i=0; streamvar[i].type > '\0'; i++)
	if (streamvar[i].type == var)
	    return streamvar[i].strformat;
    
    fatal("internal error: Type spefication for stream var '%c' not found", var);
    
    /* NOT REACHED */
    return "";
}
/* checklen(maxlen, start, extra)
 * Check if extra is allowed if we are at start and want to fit into maxlen.
 * returns false/0 if we fit
 */
int checklen(maxlen, current, extra) 
    int maxlen;
    int current;
    int extra;
{
    if ((current + extra) < maxlen) {
	return 0;
    } else {
	warning("buffer overflow: max=%d, current=%d, extra=%d",
		maxlen, current, extra);
	return 1;
    }
}
/* snpack(buffer, maxlen, type, <args>)
 * Pack multiple arguments of a certain MT_TYPE into a big-endian bytestream.
 * Return the number of bytes actually stored.  */
int snpack(char *buf, int maxlen, char *id, int type, ...)
{
    va_list ap;
    int i = 0;
    int offset = 0;
    u_int64_t q;
    u_int32_t l;
    u_int16_t c;

    if (type > MT_EOT) {
	warning("stream type (%d) out of range!", type);
	return 0;
    }

    if ( maxlen < 2 ) {
	fatal("maxlen too small!");
    } else {
	buf[offset++] = type & 0xff;
    }

    if (id) {
	if (strlen(id)+1 >= maxlen) {
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
	    bcopy(&c, buf+offset, sizeof(u_int16_t));
	    offset += sizeof(u_int16_t);
	    break;

	case 'l': 
	    l = va_arg(ap, u_int32_t);
	    l = htonl(l);
	    bcopy(&l, buf+offset, sizeof(u_int32_t));
	    offset += sizeof(u_int32_t);
	    break;

	case 'L': 
	    q = va_arg(ap, u_int64_t);
	    q = htonq(q);
	    bcopy(&q, buf+offset, sizeof(u_int64_t));
	    offset += sizeof(u_int64_t);
	    break;

	default:
	    warning("Unknown stream format identifier");
	    return 0;
	}
	i++;
    }
    va_end(ap);

    return offset;
}
/* sunpack(buffer, type, <args>)
 * Unpack multiple arguments of a certain MT_TYPE from a big-endian bytestream.
 * Return the number of bytes actually read.
 */
int sunpack(buf, ps)
    char *buf;
    struct packedstream *ps;
{
    int i=0;
    int type;
    char *in, *out;
    u_int64_t q;
    u_int32_t l;
    u_int16_t c;
    
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
	    warning("Unknown stream format identifier");
	    return 0;
	}
	i++;
    }
    return ((void*)in - (void *)buf);
}
/* psdata2strn(ps, buf, maxlen)
 * get the ascii representation of packedstream
 */
int psdata2strn(ps, buf, maxlen)
    struct packedstream *ps;
    char *buf;
    int maxlen;
{
    int i=0;
    char vartype;
    char *in, *out;
    char *formatstr;
    u_int64_t q;
    u_int32_t l;
    float c;
    
    in = (char *)(&ps->data);
    out = (char *)buf;

    while ((vartype = streamform[ps->type].form[i]) != '\0') {
	/* check buffer overflow */
	if (checklen(maxlen, (out-buf), strlenvar(vartype)))
	    return 0;
	
	formatstr = formatstrvar(vartype);

	switch (vartype) {
	case 'c':
	    c = (*((u_int16_t *)in) / 10);
	    sprintf(out, formatstr, c); 
	    in  += sizeof(u_int16_t);
	    break;

	case 'l': 
	    l = *((u_int32_t *)in);
	    sprintf(out, formatstr, l); 
	    in  += sizeof(u_int32_t);
	    break;

	case 'L': 
	    q = *((u_int64_t *)in);
	    sprintf(out, formatstr, q); 
	    in  += sizeof(u_int64_t);
	    break;

	default:
	    warning("Unknown stream format identifier");
	    return 0;
	}
	out += strlenvar(vartype);
	i++;
    }
    return (out-buf);
}
    
struct stream *create_stream(type, args)
    int type;
    char *args;
{
    struct stream *p;

    if (type < 0 || type >= MT_EOT)
	fatal("internal error: Stream type unknown.\n");

    p = (struct stream *)xmalloc(sizeof(struct stream));
    memset(p, 0, sizeof(struct stream));
    p->type = type;
    if (args != NULL) {
	p->args = xstrdup(args);
    }
    
    return p;
}

struct stream *find_source_stream(source, type, args) 
    struct source *source;
    int type;
    char *args;
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

struct stream *add_source_stream(source, type, args) 
    struct source *source;
    int type;
    char *args;
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

struct stream *find_mux_stream(mux, type, args) 
    struct mux *mux;
    int type;
    char *args;
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

struct stream *add_mux_stream(mux, type, args) 
    struct mux *mux;
    int type;
    char *args;
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

struct source *find_source(sol, name) 
    struct sourcelist *sol;
    char *name;
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

struct source *find_source_ip(sol, ip) 
    struct sourcelist *sol;
    u_int32_t ip;
{
    struct source *p;

    if (sol == NULL || SLIST_EMPTY(sol))
	return NULL;

    SLIST_FOREACH(p, sol, sources) {
	if (p->ip == ip) {
	    return p;
	}
    }

    return NULL;
}

struct source *add_source(sol, name)
    struct sourcelist *sol;
    char *name;
{
    struct source* p;

    if (sol == NULL)
	return NULL;

    if (find_source( sol, name ) != NULL)
	return NULL;

    p = (struct source *)xmalloc(sizeof(struct source));
    memset(p, 0, sizeof(struct source));
    p->name = xstrdup(name);
    SLIST_INSERT_HEAD(sol, p, sources);

    return p;
}

struct mux *find_mux(mul, name) 
    struct muxlist *mul;
    char *name;
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

struct mux *add_mux(mul, name)
    struct muxlist *mul;
    char *name;
{
    struct mux* p;

    if (mul == NULL)
	return NULL;

    if (find_mux(mul, name) != NULL)
	return NULL;

    p = (struct mux *)xmalloc(sizeof(struct mux));
    memset(p, 0, sizeof(struct mux));
    p->name = xstrdup(name);
    SLIST_INSERT_HEAD(mul, p, muxes);

    return p;
}




