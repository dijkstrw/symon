/*
 * $Id: readconf.c,v 1.2 2001/09/02 19:01:49 dijkstra Exp $
 *
 * Parse monmux.conf style configuration files 
 * 
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include "xmalloc.h"
#include "monmux.h"
#include "lex.h"
#include "error.h"
#include "net.h"

void parse_error(l, s)
    struct lex *l;
    const char *s;
{
    fatal("%s:%d:Expected %s (found '%.5s')\n", l->filename, l->cline, s, l->token);
}

int expect_token(l, op)
    struct lex *l;
    OpCodes op;
{
    const char *s;

    lex_nexttoken(l);
    if (l->op != op) {
	s = parse_opcode(op);
	parse_error(l, s);
	return 0;
    } else 
	return 1;
}

struct source *add_source(s)
    char *s;
{
    struct source* p;

    assert(s!=NULL);

    if (sources == NULL) {
	sources = xmalloc(sizeof(struct source));
	p = sources;
    } else {
	p = sources;
	while (p->next != NULL) p = p->next;
	p->next = xmalloc(sizeof(struct source));
	p = p->next;
    }
    bzero(p, sizeof(struct source));
    p->name = xstrdup(s);

    return p;
}
struct stream *find_stream(s, t, a) 
    struct source *s;
    OpCodes t;
    char *a;
{
    struct stream *p;

    assert(s != NULL);

    p = s->streams;
    for (p = s->streams; p != NULL; p = p->next)
	if ((p->type == t) 
	    && ((a == p->args) 
		|| (((void *)a != (void *)p != NULL) 
		    && strcmp(a, p->args) == 0)))
	    return p;

    return p;
}
struct stream *add_stream(s, t, a) 
    struct source *s;
    OpCodes t;
    char *a;
{
    struct stream *p;

    assert(s != NULL);

    if (find_stream(s, t, a) != NULL)
	return NULL;

    if (s->streams == NULL) {
	s->streams = xmalloc(sizeof(struct stream));
	p = s->streams;
    } else {
	p = s->streams;
	while (p->next != NULL) p = p->next;
	p->next = xmalloc(sizeof(struct stream));
	p = p->next;
    }
    bzero(p, sizeof(struct stream));
    p->type = t;
    if (a != NULL) {
	p->args = xstrdup(a);
    }

    return p;
}
/*
 * source <host> { accept ... | write ... }
 */

#define expect(x)    do {if (!expect_token(l,x)) return;} while(0);

void read_source(struct lex *l)
{
    struct source *source;
    struct stream *stream;
    char sn[_POSIX2_LINE_MAX];
    char sa[_POSIX2_LINE_MAX];
    OpCodes st;

    /* get hostname */
    lex_nexttoken(l);
    if (!lookup(l->token))
	return; 
    source = add_source(lookup_address);

    expect(oBegin);
    while (lex_nexttoken(l)) {
	switch (l->op) {
	    /* accept { cpu(x), ... } */
	case oAccept:
	    expect(oBegin);
	    while (lex_nexttoken(l) && l->op != oEnd) {
		switch (l->op) {
		case oCpu:
		case oIf:
		case oIo:
		case oMem:
		    st = l->op;
		    strncpy(&sn[0], l->token, _POSIX2_LINE_MAX);

		    /* parse arg */
		    lex_nexttoken(l);
		    if (l->op == oOpen) {
			lex_nexttoken(l);
			if (l->op == oClose) parse_error(l, "<stream argument>");
			strncpy(&sa[0], l->token, _POSIX2_LINE_MAX);
			lex_nexttoken(l);
			if (l->op != oClose) parse_error(l, ")");
		    } else {
			lex_ungettoken(l);
			sa[0]='\0';
		    }

		    if ((stream = add_stream(source, st, sa)) == NULL) {
			fatal("%s:%d:stream %s(%s) redefined.\n", 
			      l->filename, l->cline, sn, sa);
		    }

		    break; /* oCpu/oIf/oIo/oMem */
		case oComma:
		    break;
		default:
		    parse_error(l, "{cpu|mem|if|io}");
		    break;
		}
	    }
	    break; /* oAccept */
	    /* write cpu(0) in "filename" */
	case oWrite:
	    lex_nexttoken(l);
	    switch (l->op) {
	    case oCpu:
	    case oIf:
	    case oIo:
	    case oMem:
		st = l->op;
		strncpy(&sn[0], l->token, _POSIX2_LINE_MAX);
		
		/* parse arg */
		lex_nexttoken(l);
		if (l->op == oOpen) {
		    lex_nexttoken(l);
		    if (l->op == oClose) parse_error(l, "<stream argument>");
		    strncpy(&sa[0], l->token, _POSIX2_LINE_MAX);
		    lex_nexttoken(l);
		    if (l->op != oClose) parse_error(l, ")");
		} else {
		    lex_ungettoken(l);
		    sa[0]='\0';
		}
		
		expect(oIn);

		lex_nexttoken(l);
		
		if ((stream = find_stream(source, st, sa)) == NULL) {
		    if ( strlen(sa) )
			fatal("%s:%d:stream %s(%s) is not accepted for %s.\n", 
			      l->filename, l->cline, sn, sa, source->name);
		    else
			fatal("%s:%d:stream %s is not accepted for %s.\n", 
			      l->filename, l->cline, sn, source->name);
		} else {
		    int fd;
		    /* try filename */
		    if ((fd = open(l->token, O_RDWR | O_NONBLOCK, 0)) == -1) {
			fatal("%s:%d:file '%s' cannot be opened.\n", 
			      l->filename, l->cline, l->token);
		    } else {
			close( fd );
			stream->file = xstrdup(l->token);
		    }
		}
		break; /* oCpu/oIf/oIo/oMem */
	    default:
		parse_error(l, "{cpu|mem|if|io}");
		break;
	    }
	    break; /* oWrite */
	case oEnd:
	    return;
	default:
	    parse_error(l, "accept|write");
	    return;
	}
    }
}
void read_config_file(const char *filename)
{
    struct lex *l;

    l = open_lex(filename);
    
    while (lex_nexttoken(l)) {
    /* expecting keyword now */
	    switch (l->op) {
	    case oSource:
	    read_source(l);
	    break;
	default:
	    parse_error(l, "source" );
	    break;
	}
    }
    
    close_lex(l);
    exit(1);
}
