/*
 * Copyright (c) 2001-2010 Willem Dijkstra
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

#include <sys/types.h>

#include <string.h>

#include "conf.h"
#include "data.h"
#include "error.h"
#include "lex.h"
#include "net.h"
#include "symon.h"
#include "xmalloc.h"

__BEGIN_DECLS
int read_host_port(struct muxlist *, struct mux *, struct lex *);
int read_symon_args(struct mux *, struct lex *);
int read_monitor(struct muxlist *, struct lex *);
__END_DECLS

const char *default_symux_port = SYMUX_PORT;

/* parse "(ip4addr | ip6addr | hostname) [['port' | ',' ] portnumber]" */
int
read_host_port(struct muxlist * mul, struct mux * mux, struct lex * l)
{
    char muxname[_POSIX2_LINE_MAX];

    lex_nexttoken(l);
    if (!getip(l->token, AF_INET) && !getip(l->token, AF_INET6)) {
        warning("%.200s:%d: could not resolve '%.200s'",
                l->filename, l->cline, l->token);
        return 0;
    }

    mux->addr = xstrdup((const char *) &res_host);

    /* check for port statement */
    if (!lex_nexttoken(l))
        mux->port = xstrdup(default_symux_port);
    else {
        if (l->op == LXT_PORT || l->op == LXT_COMMA)
            lex_nexttoken(l);

        if (l->type != LXY_NUMBER) {
            lex_ungettoken(l);
            mux->port = xstrdup(default_symux_port);
        } else {
            mux->port = xstrdup((const char *) l->token);
        }
    }

    bzero(&muxname, sizeof(muxname));
    snprintf(&muxname[0], sizeof(muxname), "%s %s (%ds)", mux->addr, mux->port, mux->interval);
    if (rename_mux(mul, mux, muxname) == NULL) {
        warning("%.200s:%d: monitored data for host '%.200s' has already been specified",
                l->filename, l->cline, muxname);
        return 0;
    }

    return 1;
}
/* parse "resource version ['(' argument ')']", end condition == '}' */
int
read_symon_args(struct mux * mux, struct lex * l)
{
    char sn[_POSIX2_LINE_MAX];
    char sa[_POSIX2_LINE_MAX];
    int st;

    EXPECT(l, LXT_BEGIN)
        while (lex_nexttoken(l) && l->op != LXT_END) {
        switch (l->op) {
        case LXT_CPU:
        case LXT_CPUIOW:
        case LXT_DEBUG:
        case LXT_DF:
        case LXT_IF1:
        case LXT_IF:
        case LXT_IO1:
        case LXT_IO:
        case LXT_MBUF:
        case LXT_MEM1:
        case LXT_MEM:
        case LXT_PF:
        case LXT_PFQ:
        case LXT_PROC:
        case LXT_SENSOR:
        case LXT_SMART:
        case LXT_LOAD:
            st = token2type(l->op);
            strncpy(&sn[0], l->token, _POSIX2_LINE_MAX);

            /* parse arg */
            lex_nexttoken(l);
            if (l->op == LXT_OPEN) {
                lex_nexttoken(l);
                if (l->op == LXT_CLOSE) {
                    parse_error(l, "<stream argument>");
                    return 0;
                }
                strncpy(&sa[0], l->token, _POSIX2_LINE_MAX);
                lex_nexttoken(l);
                if (l->op != LXT_CLOSE) {
                    parse_error(l, ")");
                    return 0;
                }
            } else {
                lex_ungettoken(l);
                sa[0] = '\0';
            }

            if (strlen(sa) > (SYMON_PS_ARGLENV2 - 1)) {
                warning("%.200s:%d: argument '%.200s' too long for network format, "
                        "will send leading " SYMON_PS_ARGLENSTRV2 " chars only",
                        l->filename, l->cline, sa);
            }

            if ((add_mux_stream(mux, st, sa)) == NULL) {
                warning("%.200s:%d: stream %.200s(%.200s) redefined",
                        l->filename, l->cline, sn, sa);
                return 0;
            }

            break;
        case LXT_COMMA:
            break;
        default:
            parse_error(l, "{cpu|cpuiow|df|if|if1|io|io1|load|mem|mem1|pf|pfq|mbuf|debug|proc|sensor|smart}");
            return 0;
            break;
        }
    }

    return 1;
}

/* parse "'monitor' '{' resources '}' ['every' time ] 'stream' ['from' host]
 * ['to'] host [port]" */
int
read_monitor(struct muxlist * mul, struct lex * l)
{
    struct mux *mux;

    mux = add_mux(mul, SYMON_UNKMUX);

    /* parse [stream(streamarg)]+ */
    if (!read_symon_args(mux, l))
        return 0;

    lex_nexttoken(l);

    /* parse [every x seconds]? */
    if (l->op == LXT_EVERY) {
        lex_nexttoken(l);

        if (l->op == LXT_SECOND) {
            mux->interval = 1;
        } else if (l->type == LXY_NUMBER) {
            mux->interval = l->value;
            lex_nexttoken(l);
            if (l->op != LXT_SECONDS) {
                parse_error(l, "seconds");
            }
        } else {
            parse_error(l, "<number> ");
            return 0;
        }

        lex_nexttoken(l);
    } else
        mux->interval = SYMON_DEFAULT_INTERVAL;

    /* parse [stream [from <host>] to] */
    if (l->op != LXT_STREAM) {
        parse_error(l, "stream");
        return 0;
    }

    lex_nexttoken(l);

    if (l->op == LXT_FROM) {
        lex_nexttoken(l);
        /* check to see host is resolvable, result is discarded */
        if (!getip(l->token, AF_INET) && !getip(l->token, AF_INET6)) {
            warning("%.200s:%d: could not resolve '%.200s'",
                l->filename, l->cline, l->token);
            return 0;
        }
        mux->localaddr = xstrdup(l->token);
        lex_nexttoken(l);
    }

    if (l->op != LXT_TO)
        lex_ungettoken(l);

    /* parse [host [port]?] */
    return read_host_port(mul, mux, l);
}

/* Read symon.conf */
int
read_config_file(struct muxlist *muxlist, char *filename)
{
    struct lex *l;
    struct mux *mux;

    SLIST_INIT(muxlist);

    l = open_lex(filename);

    while (lex_nexttoken(l)) {
        /* expecting keyword now */
        switch (l->op) {
        case LXT_MONITOR:
            if (!read_monitor(muxlist, l))
                return 0;
            break;
        default:
            parse_error(l, "monitor");
            return 0;
            break;
        }
    }

    /* sanity checks */
    SLIST_FOREACH(mux, muxlist, muxes) {
        if (strncmp(SYMON_UNKMUX, mux->name, sizeof(SYMON_UNKMUX)) == 0) {
            /* mux was not initialised for some reason */
            return 0;
        }
        if (SLIST_EMPTY(&mux->sl)) {
            warning("%.200s: no monitors selected for mux '%.200s'",
                    l->filename, mux->name);
            return 0;
        }
    }

    if (symon_interval < SYMON_DEFAULT_INTERVAL) {
        warning("%.200s: monitoring set to every %d s", l->filename, symon_interval);
    }

    close_lex(l);

    return 1;
}
