/* $Id: lex.h,v 1.22 2005/10/16 15:26:51 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2005 Willem Dijkstra
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

#ifndef _SYMON_LIB_LEX_H
#define _SYMON_LIB_LEX_H

#include <sys/cdefs.h>

#include <stdio.h>

/* Tokens known to lex */
#define LXT_BADTOKEN   0
#define LXT_ACCEPT     1
#define LXT_BEGIN      2
#define LXT_CLOSE      3
#define LXT_COMMA      4
#define LXT_CPU        5
#define LXT_DATADIR    6
#define LXT_DEBUG      7
#define LXT_DF         8
#define LXT_END        9
#define LXT_EVERY     10
#define LXT_IF        11
#define LXT_IN        12
#define LXT_IO        13
#define LXT_IO1       14
#define LXT_MBUF      15
#define LXT_MEM       16
#define LXT_MONITOR   17
#define LXT_MUX       18
#define LXT_OPEN      19
#define LXT_PF        20
#define LXT_PFQ       21
#define LXT_PORT      22
#define LXT_PROC      23
#define LXT_SECOND    24
#define LXT_SECONDS   25
#define LXT_SENSOR    26
#define LXT_SOURCE    27
#define LXT_STREAM    28
#define LXT_TO        29
#define LXT_WRITE     30

struct lex {
    char *buffer;		/* current line(s) */
    const char *filename;
    int fh;
    char *token;		/* last token seen */
    long value;			/* value of last token seen, if num */
    int bsize;			/* size of buffer  */
    int cline;			/* current lineno */
    int curpos;			/* current position in buffer */
    int endpos;			/* current maxpos in buffer */
    int op;			/* opcode of token, if string */
    int unget;			/* bool; token pushed back */
    int tokpos;			/* current position in token buffer */
    enum {
	LXY_STRING, LXY_NUMBER, LXY_UNKNOWN
    }
	 type;			/* type of token in buffer */
};

__BEGIN_DECLS
const char *parse_opcode(int);
int lex_nexttoken(struct lex *);
int parse_token(const char *);
struct lex *open_lex(const char *);
void close_lex(struct lex *);
void lex_ungettoken(struct lex *);
void parse_error(struct lex *, const char *);
void reset_lex(struct lex *);
void rewind_lex(struct lex *);
__END_DECLS

/* EXPECT(l,x) = next token in l must be opcode x or error.  */
#define EXPECT(l, x)    do {                      \
	lex_nexttoken((l));                       \
	if ((l)->op != (x)) {                     \
	    parse_error((l), parse_opcode((x)));  \
	    return 0;                             \
	}                                         \
    } while (0);

#endif				/* _SYMON_LIB_LEX_H */
