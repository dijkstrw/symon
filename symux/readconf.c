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

#include <sys/stat.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "conf.h"
#include "data.h"
#include "error.h"
#include "lex.h"
#include "net.h"
#include "readconf.h"
#include "xmalloc.h"

__BEGIN_DECLS
int read_mux(struct muxlist * mul, struct lex *);
int read_source(struct sourcelist * sol, struct lex *, int);
int insert_filename(char *, int, int, char *);
__END_DECLS

const char *default_symux_port = SYMUX_PORT;

int
insert_filename(char *path, int maxlen, int type, char *args)
{
    int i, result;
    char *ts;
    char *ta;
    char *fta;

    fta = ts = ta = NULL;

    switch (type) {
    case MT_CPU:
        ts = "cpu";
        ta = args;
        break;
    case MT_CPUIOW:
        ts = "cpuiow";
        ta = args;
        break;
    case MT_DF:
        ts = "df_";
        ta = args;
        break;
    case MT_IF1:  /* rrd stores 64bits, if1 and if2 are equivalent */
    case MT_IF2:
        ts = "if_";
        ta = args;
        break;
    case MT_IO1:
        ts = "io1_";
        ta = args;
        break;
    case MT_IO2:
        ts = "io_";
        ta = args;
        break;
    case MT_MEM1: /* rrd stores 64bits, mem1 and mem2 are equivalent */
    case MT_MEM2:
        ts = "mem";
        ta = "";
        break;
    case MT_PF:
        ts = "pf";
        ta = "";
        break;
    case MT_PFQ:
        ts  = "pfq_";
        ta = args;
        break;
    case MT_MBUF:
        ts = "mbuf";
        ta = "";
        break;
    case MT_DEBUG:
        ts = "debug";
        ta = "";
        break;
    case MT_PROC:
        ts = "proc_";
        ta = args;
        break;
    case MT_SENSOR:
        ts = "sensor_";
        ta = args;
        break;
    case MT_SMART:
        ts = "smart_";
        ta = args;
        break;
    case MT_LOAD:
        ts = "load";
        ta = "";
        break;

    default:
        warning("%.200s:%d: internal error: type (%d) unknown",
                __FILE__, __LINE__, type);
        return 0;
    }

    /* ensure that no '/' remain in args */
    fta = xstrdup(ta);

    for (i = 0; i < strlen(fta); i++) {
        if (fta[i] == '/') fta[i] = '_';
    }

    if ((snprintf(path, maxlen, "/%s%s.rrd", ts, fta)) >= maxlen) {
        result = 0;
    } else {
        result = 1;
    }

    xfree(fta);
    return result;
}
/* parse "'mux' (ip4addr | ip6addr | hostname) [['port' | ',' portnumber]" */
int
read_mux(struct muxlist * mul, struct lex * l)
{
    char muxname[_POSIX2_LINE_MAX];
    struct mux *mux;

    if (!SLIST_EMPTY(mul)) {
        warning("%.200s:%d: only one mux statement allowed",
                l->filename, l->cline);
        return 0;
    }

    lex_nexttoken(l);
    if (!getip(l->token, AF_INET) && !getip(l->token, AF_INET6)) {
        warning("%.200s:%d: could not resolve '%s'",
                l->filename, l->cline, l->token);
        return 0;
    }

    mux = add_mux(mul, SYMON_UNKMUX);
    mux->addr = xstrdup((const char *) &res_host);

    /* check for port statement */
    lex_nexttoken(l);

    if (l->op == LXT_PORT || l->op == LXT_COMMA)
        lex_nexttoken(l);

    if (l->type != LXY_NUMBER) {
        lex_ungettoken(l);
        mux->port = xstrdup(default_symux_port);
    } else {
        mux->port = xstrdup((const char *) l->token);
    }

    bzero(&muxname, sizeof(muxname));
    snprintf(&muxname[0], sizeof(muxname), "%s %s", mux->addr, mux->port);

    if (rename_mux(mul, mux, muxname) == NULL)
        fatal("%s:%d: internal error: dual mux", __FILE__, __LINE__);

    return 1;
}
/* parse "'source' host '{' accept-stmst [write-stmts] [datadir-stmts] '}'" */
int
read_source(struct sourcelist * sol, struct lex * l, int filecheck)
{
    struct source *source;
    struct stream *stream;
    struct stat sb;
    char path[_POSIX2_LINE_MAX];
    char sn[_POSIX2_LINE_MAX];
    char sa[_POSIX2_LINE_MAX];
    int st;
    int pc;
    int fd;

    /* get hostname */
    lex_nexttoken(l);
    if (!getip(l->token, AF_INET) && !getip(l->token, AF_INET6)) {
        warning("%.200s:%d: could not resolve '%s'",
                l->filename, l->cline, l->token);
        return 0;
    }

    source = add_source(sol, res_host);

    EXPECT(l, LXT_BEGIN);
    while (lex_nexttoken(l)) {
        switch (l->op) {
            /* accept { cpu(x), ... } */
        case LXT_ACCEPT:
            EXPECT(l, LXT_BEGIN);
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
                                "will accept initial " SYMON_PS_ARGLENSTRV2 " chars only",
                                l->filename, l->cline, sa);
                        sa[SYMON_PS_ARGLENV2 - 1] = '\0';
                    }

                    if ((stream = add_source_stream(source, st, sa)) == NULL) {
                        warning("%.200s:%d: stream %.200s(%.200s) redefined",
                                l->filename, l->cline, sn, sa);
                        return 0;
                    }

                    break;      /* LXT_resource */
                case LXT_COMMA:
                    break;
                default:
                    parse_error(l, "{cpu|cpuiow|df|if|if1|io|io1|mem|mem1|pf|pfq|mbuf|debug|proc|sensor|smart|load}");
                    return 0;

                    break;
                }
            }
            break;              /* LXT_ACCEPT */
            /* datadir "path" */
        case LXT_DATADIR:
            lex_nexttoken(l);
            /* is path absolute */
            if (l->token && l->token[0] != '/') {
                warning("%.200s:%d: datadir path '%.200s' is not absolute",
                        l->filename, l->cline, l->token);
                return 0;
            }

            if (filecheck) {
                /* make sure that directory exists */
                bzero(&sb, sizeof(struct stat));

                if (stat(l->token, &sb) == 0) {
                    if (!(sb.st_mode & S_IFDIR)) {
                        warning("%.200s:%d: datadir path '%.200s' is not a directory",
                                l->filename, l->cline, l->token);
                        return 0;
                    }
                } else {
                    warning("%.200s:%d: could not stat datadir path '%.200s'",
                            l->filename, l->cline, l->token);
                    return 0;
                }
            }

            strncpy(&path[0], l->token, _POSIX2_LINE_MAX);
            path[_POSIX2_LINE_MAX - 1] = '\0';

            pc = strlen(path);

            if (path[pc - 1] == '/') {
                path[pc - 1] = '\0';
                pc--;
            }

            /* add path to empty streams */
            SLIST_FOREACH(stream, &source->sl, streams) {
                if (stream->file == NULL) {
                    if (!(insert_filename(&path[pc],
                                          _POSIX2_LINE_MAX - pc,
                                          stream->type,
                                          stream->arg))) {
                        if (stream->arg && strlen(stream->arg)) {
                            warning("%.200s:%d: failed to construct stream "
                                    "%.200s(%.200s) filename using datadir '%.200s'",
                                    l->filename, l->cline,
                                    type2str(stream->type),
                                    stream->arg, l->token);
                        } else {
                            warning("%.200s:%d: failed to construct stream "
                                    "%.200s) filename using datadir '%.200s'",
                                    l->filename, l->cline,
                                    type2str(stream->type),
                                    l->token);
                        }
                        return 0;
                    }

                    if (filecheck) {
                        /* try filename */
                        if ((fd = open(path, O_RDWR | O_NONBLOCK, 0)) == -1) {
                            /* warn, but allow */
                            warning("%.200s:%d: file '%.200s', guessed by datadir,  cannot be opened",
                                    l->filename, l->cline, path);
                        } else {
                            close(fd);
                            stream->file = xstrdup(path);
                        }
                    } else {
                        stream->file = xstrdup(path);
                    }
                }
            }
            break;              /* LXT_DATADIR */
            /* write cpu(0) in "filename" */
        case LXT_WRITE:
            lex_nexttoken(l);
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

                EXPECT(l, LXT_IN);

                lex_nexttoken(l);

                if ((stream = find_source_stream(source, st, sa)) == NULL) {
                    if (strlen(sa)) {
                        warning("%.200s:%d: stream %.200s(%.200s) is not accepted for %.200s",
                                l->filename, l->cline, sn, sa, source->addr);
                        return 0;
                    } else {
                        warning("%.200s:%d: stream %.200s is not accepted for %.200s",
                                l->filename, l->cline, sn, source->addr);
                        return 0;
                    }
                } else {
                    if (filecheck) {
                        /* try filename */
                        if ((fd = open(l->token, O_RDWR | O_NONBLOCK, 0)) == -1) {
                            warning("%.200s:%d: file '%.200s' cannot be opened",
                                    l->filename, l->cline, l->token);
                            return 0;
                        } else {
                            close(fd);

                            if (stream->file != NULL) {
                                warning("%.200s:%d: file '%.200s' overwrites previous definition '%.200s'",
                                        l->filename, l->cline, l->token, stream->file);
                                xfree(stream->file);
                            }

                            stream->file = xstrdup(l->token);
                        }
                    } else {
                        stream->file = xstrdup(l->token);
                    }
                }
                break;          /* LXT_resource */
            default:
                parse_error(l, "{cpu|cpuiow|df|if|if1|io|io1|mem|mem1|pf|pfq|mbuf|debug|proc|sensor|smart|load}");
                return 0;
                break;
            }
            break;              /* LXT_WRITE */
        case LXT_END:
            return 1;
        default:
            parse_error(l, "accept|datadir|write");
            return 0;
        }
    }

    warning("%.200s:%d: missing close brace on source statement",
            l->filename, l->cline);

    return 0;
}
/* Read symux.conf */
int
read_config_file(struct muxlist * mul, const char *filename, int filechecks)
{
    struct lex *l;
    struct source *source;
    struct stream *stream;
    struct mux *mux;
    struct sourcelist sol;
    SLIST_INIT(mul);
    SLIST_INIT(&sol);

    if ((l = open_lex(filename)) == NULL)
        return 0;

    while (lex_nexttoken(l)) {
        /* expecting keyword now */
        switch (l->op) {
        case LXT_MUX:
            if (!read_mux(mul, l)) {
                free_sourcelist(&sol);
                return 0;
            }
            break;
        case LXT_SOURCE:
            if (!read_source(&sol, l, filechecks)) {
                free_sourcelist(&sol);
                return 0;
            }
            break;
        default:
            parse_error(l, "mux|source");
            free_sourcelist(&sol);
            return 0;
            break;
        }
    }

    /* sanity checks */
    if (SLIST_EMPTY(mul)) {
        free_sourcelist(&sol);
        warning("%.200s: no mux statement seen",
                l->filename);
        return 0;
    } else {
        mux = SLIST_FIRST(mul);
        mux->sol = sol;
        if (strncmp(SYMON_UNKMUX, mux->name, sizeof(SYMON_UNKMUX)) == 0) {
            /* mux was not initialised for some reason */
            return 0;
        }
    }

    if (SLIST_EMPTY(&sol)) {
        warning("%.200s: no source section seen",
                l->filename);
        return 0;
    } else {
        SLIST_FOREACH(source, &sol, sources) {
            if (SLIST_EMPTY(&source->sl)) {
                warning("%.200s: no streams accepted for source '%.200s'",
                        l->filename, source->addr);
                return 0;
            } else {
                SLIST_FOREACH(stream, &source->sl, streams) {
                    if (stream->file == NULL) {
                        /* warn, but allow */
                        warning("%.200s: no filename specified for stream '%.200s(%.200s)' in source '%.200s'",
                                l->filename, type2str(stream->type), stream->arg, source->addr);
                    }
                }
            }
        }
    }

    close_lex(l);

    return 1;
}
