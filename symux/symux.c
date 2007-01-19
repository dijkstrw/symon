/* $Id: symux.c,v 1.37 2007/01/19 21:35:50 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2006 Willem Dijkstra
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
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>
#include <rrd.h>

#include "conf.h"
#include "data.h"
#include "error.h"
#include "limits.h"
#include "symux.h"
#include "symuxnet.h"
#include "net.h"
#include "readconf.h"
#include "share.h"
#include "xmalloc.h"

__BEGIN_DECLS
void exithandler();
void huphandler(int);
void signalhandler(int);
__END_DECLS

int flag_hup = 0;
fd_set fdset;
int maxfd;

void
exithandler(int s)
{
    info("received signal %d - quitting", s);
    exit(EX_TEMPFAIL);
}
void
huphandler(int s)
{
    info("hup received");
    flag_hup = 1;
}
/*
 * symux is the receiver of symon performance measurements.
 *
 * The main goals symon hopes to accomplish is:
 * - to take fine grained measurements of system parameters
 * - with minimal performance impact
 * - in a secure way.
 *
 * Measuring system parameters (e.g. interfaces) sometimes means traversing
 * lists in kernel memory. Because of this the measurement of data has been
 * decoupled from the processing and storage of data. Storing the measured
 * information that symon provides is done by a second program, called symux.
 *
 * Symon can keep track of cpu, memory, disk and network interface
 * interactions. Symon was built specifically for OpenBSD.
 */
int
main(int argc, char *argv[])
{
    struct symonpacket packet;
    struct packedstream ps;
    char *cfgfile;
    char *cfgpath;
    char *stringbuf;
    char *stringptr;
    int maxstringlen;
    struct muxlist mul, newmul;
    char *arg_ra[4];
    struct stream *stream;
    struct source *source;
    struct sourcelist *sol;
    struct mux *mux;
    FILE *f;
    int ch;
    int churnbuflen;
    int flag_list;
    int offset;
    int result;
    unsigned int rrderrors;
    int slot;
    time_t timestamp;

    SLIST_INIT(&mul);

    /* reset flags */
    flag_debug = 0;
    flag_daemon = 0;
    flag_list = 0;

    cfgfile = SYMUX_CONFIG_FILE;

    while ((ch = getopt(argc, argv, "df:lv")) != -1) {
        switch (ch) {
        case 'd':
            flag_debug = 1;
            break;
        case 'f':
            if (optarg && optarg[0] != '/') {
                /* cfg path needs to be absolute, we will be a daemon soon */
                cfgpath = xmalloc(MAXPATHLEN);
                if ((cfgpath = getcwd(cfgpath, MAXPATHLEN)) == NULL)
                    fatal("could not get working directory");

                maxstringlen = strlen(cfgpath) + strlen(optarg) + 1;
                cfgfile = xmalloc(maxstringlen);
                strncpy(cfgfile, cfgpath, maxstringlen);
                stringptr = cfgfile + strlen(cfgpath);
                stringptr[0] = '/';
                stringptr++;
                strncpy(stringptr, optarg, maxstringlen - (cfgfile - stringptr));
                cfgfile[maxstringlen] = '\0';

                free(cfgpath);
            } else
                cfgfile = xstrdup(optarg);

            break;
        case 'l':
            flag_list = 1;
            break;
        case 'v':
            info("symux version %s", SYMUX_VERSION);
        default:
            info("usage: %s [-d] [-l] [-v] [-f cfgfile]", __progname);
            exit(EX_USAGE);
        }
    }

    if (flag_list == 1) {
        /* read configuration without file checks */
        result = read_config_file(&mul, cfgfile, 0);
        if (!result) {
            fatal("configuration contained errors; quitting");
        }

        mux = SLIST_FIRST(&mul);
        if (mux == NULL) {
            fatal("%s:%d: mux not found", __FILE__, __LINE__);
        }

        sol = &mux->sol;

        if (sol == NULL) {
            fatal("%s:%d: sourcelist not found", __FILE__, __LINE__);
        }

        SLIST_FOREACH(source, sol, sources) {
            if (! SLIST_EMPTY(&source->sl)) {
                SLIST_FOREACH(stream, &source->sl, streams) {
                    if (stream->file != NULL) {
                        info("%.200s", stream->file);
                    }
                }
            }
        }
        return (EX_OK);
    } else {
        /* read configuration file with file access checks */
        result = read_config_file(&mul, cfgfile, 1);
        if (!result) {
            fatal("configuration contained errors; quitting");
        }
    }


    setegid(getgid());
    setgid(getgid());

    if (flag_debug != 1) {
        if (daemon(0, 0) != 0)
            fatal("daemonize failed");

        flag_daemon = 1;

        /* record pid */
        f = fopen(SYMUX_PID_FILE, "w");
        if (f) {
            fprintf(f, "%u\n", (u_int) getpid());
            fclose(f);
        }
    }

    info("symux version %s", SYMUX_VERSION);

    if (flag_debug == 1)
        info("program id=%d", (u_int) getpid());

    mux = SLIST_FIRST(&mul);

    churnbuflen = calculate_churnbuffer(&mux->sol);
    debug("size of churnbuffer = %d", churnbuflen);
    initshare(churnbuflen);

    /* catch signals */
    signal(SIGHUP, huphandler);
    signal(SIGINT, exithandler);
    signal(SIGQUIT, exithandler);
    signal(SIGTERM, exithandler);
    signal(SIGTERM, exithandler);

    /* prepare crc32 */
    init_crc32();

    /* prepare sockets */
    if (get_symon_sockets(mux) == 0)
        fatal("no sockets could be opened for incoming symon traffic");
    if (get_client_socket(mux) == 0)
        fatal("socket for client connections could not be opened");

    rrderrors = 0;
    /* main loop */
    for (;;) {                  /* FOREVER */
        wait_for_traffic(mux, &source, &packet);

        if (flag_hup == 1) {
            flag_hup = 0;

            SLIST_INIT(&newmul);

            if (!read_config_file(&newmul, cfgfile, 1)) {
                info("new configuration contains errors; keeping old configuration");
                free_muxlist(&newmul);
            } else {
                info("read configuration file '%.100s' successfully", cfgfile);
                free_muxlist(&mul);
                mul = newmul;
                mux = SLIST_FIRST(&mul);
                get_symon_sockets(mux);
                get_client_socket(mux);
            }
        } else {

            /*
             * Put information from packet into stringbuf (shared region).
             * Note that the stringbuf is used twice: 1) to update the
             * rrdfile and 2) to collect all the data from a single packet
             * that needs to shared to the clients. This is the reason for
             * the hasseling with stringptr.
             */

            offset = mux->offset;
            maxstringlen = shared_getmaxlen();
            /* put time:ip: into shared region */
            slot = master_forbidread();
            timestamp = (time_t) packet.header.timestamp;
            stringbuf = shared_getmem(slot);
            debug("stringbuf = 0x%8x", stringbuf);
            snprintf(stringbuf, maxstringlen, "%s;", source->addr);

            /* hide this string region from rrd update */
            maxstringlen -= strlen(stringbuf);
            stringptr = stringbuf + strlen(stringbuf);

            while (offset < packet.header.length) {
                bzero(&ps, sizeof(struct packedstream));
                offset += sunpack(packet.data + offset, &ps);

                /* find stream in source */
                stream = find_source_stream(source, ps.type, ps.arg);

                if (stream != NULL) {
                    /* put type and arg in and hide from rrd */
                    snprintf(stringptr, maxstringlen, "%s:%s:", type2str(ps.type), ps.arg);
                    maxstringlen -= strlen(stringptr);
                    stringptr += strlen(stringptr);
                    /* put timestamp in and show to rrd */
                    snprintf(stringptr, maxstringlen, "%u", (unsigned int)timestamp);
                    arg_ra[3] = stringptr;
                    maxstringlen -= strlen(stringptr);
                    stringptr += strlen(stringptr);

                    /* put measurements in */
                    ps2strn(&ps, stringptr, maxstringlen, PS2STR_RRD);

                    if (stream->file != NULL) {
                        /* clear optind for getopt call by rrdupdate */
                        optind = 0;
                        /* save if file specified */
                        arg_ra[0] = "rrdupdate";
                        arg_ra[1] = "--";
                        arg_ra[2] = stream->file;

                        /*
                         * This call will cost a lot (symux will become
                         * unresponsive and eat up massive amounts of cpu) if
                         * the rrdfile is out of sync.
                         */
                        rrd_update(4, arg_ra);

                        if (rrd_test_error()) {
                            if (rrderrors < SYMUX_MAXRRDERRORS) {
                                rrderrors++;
                                warning("rrd_update:%.200s", rrd_get_error());
                                warning("%.200s %.200s %.200s %.200s", arg_ra[0], arg_ra[1],
                                        arg_ra[2], arg_ra[3]);
                                if (rrderrors == SYMUX_MAXRRDERRORS) {
                                    warning("maximum rrd errors reached - will stop reporting them");
                                }
                            }
                            rrd_clear_error();
                        } else {
                            if (flag_debug == 1)
                                debug("%.200s %.200s %.200s %.200s", arg_ra[0], arg_ra[1],
                                      arg_ra[2], arg_ra[3]);
                        }
                    }
                    maxstringlen -= strlen(stringptr);
                    stringptr += strlen(stringptr);
                    snprintf(stringptr, maxstringlen, ";");
                    maxstringlen -= strlen(stringptr);
                    stringptr += strlen(stringptr);
                } else {
                    debug("ignored unaccepted stream %.16s(%.16s) from %.20s", type2str(ps.type),
                          ((ps.arg == NULL) ? "0" : ps.arg), source->addr);
                }
            }
            /*
             * packet = parsed and in ascii in shared region -> copy to
             * clients
             */
            snprintf(stringptr, maxstringlen, "\n");
            stringptr += strlen(stringptr);
            shared_setlen(slot, (stringptr - stringbuf));
            debug("churnbuffer used: %d", (stringptr - stringbuf));
            master_permitread();
        }                       /* flag_hup == 0 */
    }                           /* forever */

    /* NOT REACHED */
    return (EX_SOFTWARE);
}
