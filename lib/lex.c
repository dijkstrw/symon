/* $Id: lex.c,v 1.28 2007/02/11 20:07:31 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2007 Willem Dijkstra
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
 * This lexical analyser was written to be smaller than flex and with less
 * features. Its attributes in random order: capable of multiple instances, one
 * token lookahead, strings delimited by ' or ", comments can start anywhere
 * with # and last until eol, max token size = _POSIX2_LINE_LENGTH. Tokens are
 * defined in lex.h, the mapping of tokens to ascii happens here.
 *
 * Usage:
 *
 *    l = open_lex(filename);
 *    while (lex_nexttoken(l)) {
 *       use l->token, l->op, l->value
 *    }
 *    close_lex(l);
 */

#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "xmalloc.h"
#include "lex.h"
#include "error.h"

static struct {
    const char *name;
    int opcode;
}      keywords[] = {
    { "{", LXT_BEGIN },
    { "}", LXT_END },
    { "(", LXT_OPEN },
    { ")", LXT_CLOSE },
    { ",", LXT_COMMA },
    { "accept", LXT_ACCEPT },
    { "cpu", LXT_CPU },
    { "datadir", LXT_DATADIR },
    { "debug", LXT_DEBUG },
    { "df", LXT_DF },
    { "every", LXT_EVERY },
    { "from", LXT_FROM },
    { "if", LXT_IF },
    { "in", LXT_IN },
    { "io", LXT_IO },
    { "io1", LXT_IO1 },
    { "io2", LXT_IO },
    { "mbuf", LXT_MBUF },
    { "mem", LXT_MEM },
    { "monitor", LXT_MONITOR },
    { "mux", LXT_MUX },
    { "pf", LXT_PF },
    { "pfq", LXT_PFQ },
    { "port", LXT_PORT },
    { "proc", LXT_PROC },
    { "second", LXT_SECOND },
    { "seconds", LXT_SECONDS },
    { "sensor", LXT_SENSOR },
    { "source", LXT_SOURCE },
    { "stream", LXT_STREAM },
    { "to", LXT_TO },
    { "write", LXT_WRITE },
    { NULL, 0 }
};
#define KW_OPS "{},()"

/* Return the number of the token pointed to by cp or LXT_BADTOKEN */
int
parse_token(const char *cp)
{
    u_int i;

    for (i = 0; keywords[i].name; i++)
        if (strcasecmp(cp, keywords[i].name) == 0)
            return keywords[i].opcode;

    return LXT_BADTOKEN;
}
/* Return the ascii representation of an opcode */
char *
parse_opcode(const int op)
{
    u_int i;

    for (i = 0; keywords[i].name; i++)
        if (keywords[i].opcode == op)
            return (char *) keywords[i].name;

    return NULL;
}
/* Read a line and increase buffer if needed */
int
lex_readline(struct lex *l)
{
    char *bp;

    if (l == NULL)
        return 0;

    bp = l->buffer;

    if (l->buffer) {
        l->curpos = 0;
        l->endpos = 0;
    } else {
        l->bsize = _POSIX2_LINE_MAX;
        l->buffer = xmalloc(l->bsize);
        bp = l->buffer;
    }

    l->endpos = read(l->fh, bp, (l->buffer + l->bsize) - bp);

    return (l->endpos > 0);
}
/* Copy char out of input stream */
void
lex_copychar(struct lex *l)
{
    if (l == NULL)
        return;

    l->token[l->tokpos] = l->buffer[l->curpos];

    if (++l->tokpos >= _POSIX2_LINE_MAX) {
        l->token[_POSIX2_LINE_MAX - 1] = '\0';
        fatal("%.200s:%d: parse error at '%.200s'", l->filename, l->cline, l->token);
        /* NOT REACHED */
    }
}
/* Get next char, read next line if needed */
int
lex_nextchar(struct lex *l)
{
    if (l == NULL)
        return 0;

    l->curpos++;

    if (l->curpos >= l->endpos)
        if (!lex_readline(l))
            return 0;

    if (l->buffer[l->curpos] == '\n')
        l->cline++;

    return 1;
}
/* Close of current token with a '\0' */
void
lex_termtoken(struct lex *l)
{
    if (l == NULL)
        return;

    l->token[l->tokpos] = l->token[_POSIX2_LINE_MAX - 1] = '\0';
    l->tokpos = 0;
}
/* Unget token; the lexer allows 1 look a head. */
void
lex_ungettoken(struct lex *l)
{
    if (l == NULL)
        return;

    l->unget = 1;
}
/* Get the next token in lex->token. return 0 if no more tokens found. */
int
lex_nexttoken(struct lex *l)
{
    if (l == NULL)
        return 0;

    /* return same token as last time if it has been pushed back */
    if (l->unget) {
        l->unget = 0;
        return 1;
    }

    l->op = LXT_BADTOKEN;
    l->value = 0;
    l->type = LXY_UNKNOWN;

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
        } else if (!lex_nextchar(l))
            return 0;
    }

    l->type = LXY_STRING;

    /* "delimited string" */
    if (l->buffer[l->curpos] == '"') {
        if (!lex_nextchar(l)) {
            warning("%.200s:%d: unbalanced '\"'", l->filename, l->cline);
            return 0;
        }
        while (l->buffer[l->curpos] != '"') {
            lex_copychar(l);
            if (!lex_nextchar(l)) {
                warning("%.200s:%d: unbalanced '\"'", l->filename, l->cline);
                return 0;
            }
        }
        lex_termtoken(l);
        lex_nextchar(l);
        return 1;
    }

    /* 'delimited string' */
    if (l->buffer[l->curpos] == '\'') {
        if (!lex_nextchar(l)) {
            warning("%.200s:%d: unbalanced \"\'\"", l->filename, l->cline);
            return 0;
        }
        while (l->buffer[l->curpos] != '\'') {
            lex_copychar(l);
            if (!lex_nextchar(l)) {
                warning("%.200s:%d: unbalanced \"\'\"", l->filename, l->cline);
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
    lex_termtoken(l);
    l->op = parse_token(l->token);

    /* number */
    if (l->token[0] >= '0' && l->token[0] <= '9') {
        if (strlen(l->token) == strspn(l->token, "0123456789")) {
            l->type = LXY_NUMBER;
            l->value = strtol(l->token, NULL, 10);
        }
    }
    return 1;
}
/* Create and initialize a lexical analyser */
struct lex *
open_lex(const char *filename)
{
    struct lex *l;

    l = xmalloc(sizeof(struct lex));
    reset_lex(l);
    l->buffer = NULL;
    l->filename = filename;
    l->token = xmalloc(_POSIX2_LINE_MAX);

    if ((l->fh = open(l->filename, O_RDONLY)) < 0) {
        warning("could not open file \"%.200s\":%.200s",
                l->filename, strerror(errno));
        close_lex(l);
        return NULL;
    }

    lex_readline(l);
    return l;
}
/* Prepare file for another lexer run */
void
rewind_lex(struct lex *l)
{
    off_t filepos;

    if (l == NULL)
        return;

    reset_lex(l);

    if ((filepos = lseek(l->fh, (off_t)0, SEEK_SET)) == -1) {
        warning("could not rewind file '%.200s':%.200s",
                l->filename, strerror(errno));
    }
}
/* Reset lexer to start of file defaults */
void
reset_lex(struct lex *l)
{
    if (l == NULL)
        return;

    l->cline = 1;
    l->curpos = 0;
    l->endpos = 0;
    l->op = LXT_BADTOKEN;
    l->tokpos = 0;
    l->type = LXY_UNKNOWN;
    l->unget = 0;
    l->value = 0;
}
/* Destroy a lexical analyser */
void
close_lex(struct lex *l)
{
    if (l == NULL)
        return;

    if (l->fh)
        close(l->fh);
    if (l->buffer)
        xfree(l->buffer);
    if (l->token)
        xfree(l->token);
    xfree(l);
}
/* Signal a parse error */
void
parse_error(struct lex *l, const char *s)
{
    warning("%.200s:%d: expected '%.200s' found '%.8s'",
            l->filename, l->cline, s, l->token);
}
