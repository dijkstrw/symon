/*
 * $Id: lex.c,v 1.2 2001/09/02 19:01:49 dijkstra Exp $
 *
 * Simple lexical analyser:
 *
 * Usage:
 * l = open_lex(filename); 
 * while (lex_nexttoken(l)) {
 *     use l->token, l->op, l->value
 * }
 * close_lex(l);
 *
 * Attributes:
 * - push back one token
 * - keywords are in lex.h
 * - strings are delimited by " or '
 * - '\n' like statements are not parsed
 * - comments start with # and last until eol
 * - max token size = _POSIX2_LINE_LENGTH
 */
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <limits.h>
#include <stdlib.h>
#include "xmalloc.h"
#include "lex.h"
#include "error.h"

static struct {
    const char *name;
    OpCodes opcode;
} keywords[] = {
    { "source", oSource },
    { "accept", oAccept },
    { "write", oWrite },
    { "in", oIn },
    { "cpu", oCpu },
    { "mem", oMem },
    { "if", oIf },
    { "io", oIo },
    { "{", oBegin },
    { "}", oEnd },
    { ",", oComma },
    { "(", oOpen },
    { ")", oClose },
    { NULL, 0 }
};
#define KW_OPS "{},()"

/*
 * Returns the number of the token pointed to by cp or oBadToken
 */
OpCodes parse_token(cp)
    const char *cp;
{
    u_int i;

    for (i = 0; keywords[i].name; i++)
	if (strcasecmp(cp, keywords[i].name) == 0)
	    return keywords[i].opcode;
        
    return oBadToken;
}
/*
 * Returns the ascii representation of an opcode
 */
const char* parse_opcode(op)
    const OpCodes op;
{
    u_int i;
    
    for (i=0; keywords[i].name; i++)
	if (keywords[i].opcode == op)
	    return keywords[i].name;

    return keywords[i].name; /* NULL */
}
/*
 * Read a line from the configuration file and allocate extra room if
 * needed 
 */
int lex_readline(l)
    struct lex *l;
{
    char *bp;

    bp = l->buffer;
    
    if (l->buffer) {
	if ((l->curpos < l->endpos) && ((l->bsize - l->endpos) < _POSIX2_LINE_MAX)) {
	    l->bsize += _POSIX2_LINE_MAX;
	    l->buffer = xrealloc(l->buffer, l->bsize);
	    bp += l->endpos;
	} else {
	    l->curpos = 0;
	    l->endpos = 0;
	}
    } else {
	l->bsize = _POSIX2_LINE_MAX;
	l->buffer = xmalloc(l->bsize);
	bp = l->buffer;
    }
    
    if (!fgets(bp, (l->buffer+l->bsize)-bp, l->fh))
	return 0;
    else {
	l->endpos += strlen(bp) - 1;
	return 1;
    }
}
/*
 * Tokens are copied out of the input stream. This allows us to deal with
 * successive tokens without whitespace in the middle.  
*/
void lex_copychar(l) 
    struct lex *l;
{
    l->token[l->tokpos]=l->buffer[l->curpos];
    
    if (++l->tokpos >= _POSIX2_LINE_MAX) {
	l->token[_POSIX2_LINE_MAX-1] = '\0';
	fatal("%s:%d:Parse error at '%s'.", l->filename, l->cline, l->token);
	/* NOT REACHED */
    }
}
int lex_nextchar(l)
    struct lex *l;
{
    l->curpos++;

    /* read line if needed */
    if (l->curpos > l->endpos)
	if (!lex_readline(l))
	    return 0;

    if (l->buffer[l->curpos] == '\n') l->cline++;
    
    return 1;
}
/*
 * Close of current token with a '\0'
 */
void lex_termtoken(l)
    struct lex *l;
{
    l->token[l->tokpos] = l->token[_POSIX2_LINE_MAX-1] = '\0';
    l->tokpos=0;
}
void lex_ungettoken(l) 
    struct lex *l;
{
    l->unget = 1;
}
/*
 * Get the next token in lex->token. return 0 if no more tokens found.
 */
int lex_nexttoken(l)
    struct lex *l;
{
    char *e;

    /* return same token as last time if it has been pushed back */
    if (l->unget) {
	l->unget = 0;
	return 1;
    }
	
    l->op = oBadToken;

    /* find first non whitespace */
    while (l->buffer[l->curpos] == ' ' || 
	   l->buffer[l->curpos] == '\t' || 
	   l->buffer[l->curpos] == '\r' ||
	   l->buffer[l->curpos] == '\n' ||
	   l->buffer[l->curpos] == '\0' ||
	   l->buffer[l->curpos] == '#') {
	/* flush rest of line if comment */
	if (l->buffer[l->curpos] == '#') {
	    while (l->buffer[l->curpos] != '\n') 
		if (!lex_nextchar(l))
		    return 0;
	} else
	    if (!lex_nextchar(l))
		return 0;
    }
    
    /* number */
    if (l->buffer[l->curpos] >= '0' 
	&& l->buffer[l->curpos] <= '9') { 
	l->value = strtol(&l->buffer[l->curpos], &e, 10);
	snprintf(l->token, _POSIX2_LINE_MAX, "%ld", l->value);
	l->tokpos = strlen(l->token);
	l->type = tNumber;
	l->curpos = e - l->buffer;
	lex_termtoken(l);
	return 1;
    } 

    l->type = tString;

    /* delimited string */
    if (l->buffer[l->curpos] == '"') {
	if (!lex_nextchar(l)) {
	    warning("%s:%d:unbalanced '\"'", l->filename, l->cline);
	    return 0;
	}
	while (l->buffer[l->curpos] != '"') {
	    lex_copychar(l);
	    if (!lex_nextchar(l)) {
		warning("%s:%d:unbalanced '\"'", l->filename, l->cline);
		return 0;
	    }
	}
	lex_termtoken(l);
	lex_nextchar(l);
	return 1;
    }
    
    /* delimited string 2 */
    if (l->buffer[l->curpos] == '\'') {
	if (!lex_nextchar(l)) {
	    warning("%s:%d:unbalanced \"\'\"", l->filename, l->cline);
	    return 0;
	}
	while (l->buffer[l->curpos] != '\'') {
	    lex_copychar(l);
	    if (!lex_nextchar(l)) {
		warning("%s:%d:unbalanced \"\'\"", l->filename, l->cline);
		return 0;
	    }
	}
	lex_termtoken(l);
	lex_nextchar(l);
	return 1;
    }
    
    /* one char keyword */
    if (strchr(KW_OPS, l->buffer[l->curpos])) {
	lex_copychar(l);
	lex_termtoken(l);
	l->op = parse_token(l->token);
	lex_nextchar(l);
	return 1;
    }

    /* single keyword */
    while (l->buffer[l->curpos] != ' ' &&
	   l->buffer[l->curpos] != '\t' && 
	   l->buffer[l->curpos] != '\r' &&
	   l->buffer[l->curpos] != '\n' &&
	   l->buffer[l->curpos] != '\0' &&
	   l->buffer[l->curpos] != '#' &&
	   (strchr(KW_OPS, l->buffer[l->curpos]) == NULL)) {
	lex_copychar(l);
	if (!lex_nextchar(l))
	    break;
    }
    
    /* flush rest of line if comment */
    if (l->buffer[l->curpos] == '#') {
	lex_termtoken(l);
	while (l->buffer[l->curpos] != '\n') 
	    if (!lex_nextchar(l))
		break;
    } else {
	lex_termtoken(l);
    }

    l->op = parse_token(l->token);
    return 1;
}    
/*
 * Create and initialize a lexical analyser
 */
struct lex *open_lex(filename)
    const char *filename;
{
    struct lex *l;
    
    l = xmalloc(sizeof(struct lex));
    l->filename = filename;
    l->buffer = NULL;
    l->curpos = 0;
    l->endpos = 0;
    l->unget = 0;
    l->cline = 1;
    l->type = tNumber;
    l->value = 0;
    l->token = xmalloc(_POSIX2_LINE_MAX);
    l->tokpos = 0;
    l->op = oBadToken;

    if ((l->fh = fopen(l->filename, "r")) == NULL) {
	fatal("Could not open file '%s':%s", 
	     l->filename,strerror(errno));
	/* NOT REACHED */
    }

    lex_nextchar(l);
    return l;
}
void close_lex(l)
    struct lex *l;
{
    if (l == NULL) return;
    if (l->fh) fclose(l->fh);
    if (l->buffer) xfree(l->buffer);
    if (l->token) xfree(l->token);
    xfree(l);
}
