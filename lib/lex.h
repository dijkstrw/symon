/* $Id: lex.h,v 1.8 2002/04/01 20:15:55 dijkstra Exp $ */

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

/*
 * This file defines the keyword tokens and lexer structure for the simple
 * lexical analyser. 
 */

#ifndef _MON_LIB_LEX_H
#define _MON_LIB_LEX_H

#include <sys/cdefs.h>

#include <stdio.h>

/* Tokens known to lex. */
#define LXT_BADTOKEN   0
#define LXT_MEM        1
#define LXT_IF         2
#define LXT_IO         3
#define LXT_CPU        4
#define LXT_SOURCE     5
#define LXT_HUB        6
#define LXT_STREAM     7
#define LXT_PORT       8
#define LXT_ACCEPT     9
#define LXT_WRITE     10
#define LXT_IN        11
#define LXT_BEGIN     12
#define LXT_END       13
#define LXT_COMMA     14
#define LXT_OPEN      15
#define LXT_CLOSE     16
#define LXT_COLON     17

struct lex {
    char *buffer;          /* current line(s) */
    const char *filename;
    FILE *fh;
    char *token;           /* last token seen */
    long value;            /* value of last token seen, if num */
    int bsize;             /* size of buffer  */
    int cline;             /* current lineno */
    int curpos;            /* current position in buffer */
    int endpos;            /* current maxpos in buffer */
    int op;                /* opcode of token, if string */
    int unget;             /* bool; token pushed back */
    int tokpos;            /* current position in token buffer */
    enum { LXY_STRING, LXY_NUMBER, LXY_UNKNOWN } 
        type;              /* type of token in buffer */
};

__BEGIN_DECLS
struct lex *open_lex(const char *);
void        close_lex(struct lex *);
int         lex_nexttoken(struct lex *);
void        lex_ungettoken(struct lex *);
const char* parse_opcode(int);
int         parse_token(const char *);
void        parse_error(struct lex *, const char *);
__END_DECLS

/* EXPECT(x) = next token in l must be x or return.  */
#define EXPECT(x)    do {                         \
	lex_nexttoken(l);                         \
	if (l->op != (x)) {                       \
	    parse_error(l, parse_opcode((x)));    \
	    return 0;                             \
	}                                         \
    } while (0);

#endif /*_MON_LIB_LEX_H*/

