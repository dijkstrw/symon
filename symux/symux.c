/* $Id: symux.c,v 1.21 2002/09/02 06:17:37 dijkstra Exp $ */

/*
 * Copyright (c) 2001-2002 Willem Dijkstra
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <machine/endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>
#include <rrd.h>

#include "data.h"
#include "error.h"
#include "limits.h"
#include "monmux.h"
#include "muxnet.h"
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
exithandler(int s) {
    info("received signal %d - quitting", s);
    exit(EX_TEMPFAIL);
}
void
huphandler(int s) {
    info("sighup (%d) received", s);
    flag_hup = 1;
}
/* 
 * Monmux is the receiver of mon performance measurements.
 *
 * The main goals mon hopes to accomplish is:
 * - to take fine grained measurements of system parameters 
 * - with minimal performance impact 
 * - in a secure way.
 * 
 * Measuring system parameters (e.g. interfaces) sometimes means traversing
 * lists in kernel memory. Because of this the measurement of data has been
 * decoupled from the processing and storage of data. Storing the measured
 * information that mon provides is done by a second program, called monmux.
 * 
 * Mon can keep track of cpu, memory, disk and network interface
 * interactions. Mon was built specifically for OpenBSD.
 */
int 
main(int argc, char *argv[])
{
    struct monpacket packet;
    struct packedstream ps;
    char *cfgfile;
    char *cfgpath;
    char *stringbuf;
    char *stringptr;
    int maxstringlen;
    struct muxlist mul, newmul;
    struct sourcelist sol, newsol;
    char *arg_ra[4];
    struct stream *stream;
    struct source *source;
    struct mux *mux;
    FILE *f;
    int ch;
    int offset;
    time_t timestamp;
    int churnbuflen;

    SLIST_INIT(&mul);
    SLIST_INIT(&sol);
	
    /* reset flags */
    flag_debug = 0;
    flag_daemon = 0;
    
    cfgfile = MONMUX_CONFIG_FILE;
    while ((ch = getopt(argc, argv, "dvf:")) != -1) {
	switch (ch) {
	case 'd':
	    flag_debug = 1;
	    break;
	case 'f':
	    if (optarg && optarg[1] != '/') {
		/* cfg path needs to be absolute, we will be a daemon soon */
		if ((cfgpath = getwd(NULL)) == NULL)
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
	case 'v':
	    info("monmux version %s", MONMUX_VERSION);
	default:
	    info("usage: %s [-d] [-v] [-f cfgfile]", __progname);
	    exit(EX_USAGE);
	}
    }

    if (flag_debug != 1) {
	if (daemon(0,0) != 0)
	    fatal("daemonize failed");
	
	flag_daemon = 1;

	/* record pid */
	f = fopen(MONMUX_PID_FILE, "w");
	if (f) {
	    fprintf(f, "%u\n", (u_int) getpid());
	    fclose(f);
	}
    } 
    
    info("monmux version %s", MONMUX_VERSION);

    /* parse configuration file */
    if (!read_config_file(&mul, &sol, cfgfile))
	fatal("configuration contained errors; quitting");


    if (flag_debug == 1)
	info("program id=%d", (u_int) getpid());

    mux = SLIST_FIRST(&mul);

    churnbuflen = calculate_churnbuffer(&sol);
    debug("size of churnbuffer = %d", churnbuflen);
    initshare(churnbuflen);

    /* catch signals */
    signal(SIGHUP, huphandler);
    signal(SIGINT, exithandler); 
    signal(SIGQUIT, exithandler); 
    signal(SIGTERM, exithandler);
    signal(SIGTERM, exithandler); 

    /* Prepare crc32 */
    init_crc32();

    getmonsocket(mux);
    getclientsocket(mux);

    for (;;) {
	waitfortraffic(mux, &sol, &source, &packet);

	if (flag_hup == 1) {
	    flag_hup = 0;

	    SLIST_INIT(&newmul);
	    SLIST_INIT(&newsol);

	    if (!read_config_file(&newmul, &newsol, cfgfile)) {
		info("new configuration contains errors; keeping old configuration");
		free_muxlist(&newmul);
		free_sourcelist(&newsol);
	    } else {
		info("read configuration file succesfully");
		free_muxlist(&mul);
		free_sourcelist(&sol);
		mul = newmul;
		sol = newsol;
		mux = SLIST_FIRST(&mul);
		getmonsocket(mux);
		getclientsocket(mux);
	    }
	    break; /* wait for next alarm */
	} 

	/* Put information from packet into stringbuf (shared region). Note
	 * that the stringbuf is used twice: 1) to update the rrdfile and 2) to
	 * collect all the data from a single packet that needs to shared to
	 * the clients. This is the reason for the hasseling with stringptr.
	 */
	
	offset = mux->offset;
	maxstringlen = shared_getmaxlen();
	/* put time:ip: into shared region */
	master_forbidread();
	timestamp = (time_t) packet.header.timestamp;
	stringbuf = (char *)shared_getmem();
	snprintf(stringbuf, maxstringlen, "%u.%u.%u.%u;",
		 IPAS4BYTES(source->ip));
	
	/* hide this string region from rrd update */
	maxstringlen -= strlen(stringbuf);
	stringptr = stringbuf + strlen(stringbuf);
	
	while (offset < packet.header.length) {
	    bzero(&ps, sizeof(struct packedstream));
	    offset += sunpack(packet.data + offset, &ps);
	    
	    /* find stream in source */
	    stream = find_source_stream(source, ps.type, ps.args);
	    
	    if (stream != NULL) {
		/* put type in and hide from rrd */
		snprintf(stringptr, maxstringlen, "%s:", type2str(ps.type));
		maxstringlen -= strlen(stringptr);
		stringptr += strlen(stringptr);
		/* put arguments in and hide from rrd */
		snprintf(stringptr, maxstringlen, "%s:", 
			 ((ps.args == NULL) ? "0" : ps.args));
		maxstringlen -= strlen(stringptr);
		stringptr += strlen(stringptr);
		/* put timestamp in and show to rrd */
		snprintf(stringptr, maxstringlen, "%u", timestamp);
		arg_ra[3] = stringptr;
		maxstringlen -= strlen(stringptr);
		stringptr += strlen(stringptr);
		
		/* put measurements in */
		ps2strn(&ps, stringptr, maxstringlen, PS2STR_RRD);
		
		if (stream->file != NULL) {
		    /* save if file specified */
		    arg_ra[0] = "rrdupdate";
		    arg_ra[1] = "--";
		    arg_ra[2] = stream->file;
		    
		    /* This call will cost a lot (monmux will become
		     * unresponsive and eat up massive amounts of cpu) if
		     * the rrdfile is out of sync. While I could update the
		     * rrd in a separate process, I choose not to at this
		     * time.  
		     */
		    rrd_update(4, arg_ra);
		    
		    if (rrd_test_error()) {
			warning("rrd_update:%s", rrd_get_error());
			warning("%s %s %s %s", arg_ra[0], arg_ra[1], 
				arg_ra[2], arg_ra[3]);
			rrd_clear_error();                                                            
		    } else {
			if (flag_debug == 1) 
			    debug("%s %s %s %s", arg_ra[0], arg_ra[1], 
				  arg_ra[2], arg_ra[3]);
		    }
		}
		maxstringlen -= strlen(stringptr);
		stringptr += strlen(stringptr);
		snprintf(stringptr, maxstringlen, ";");
		maxstringlen -= strlen(stringptr);
		stringptr += strlen(stringptr);
	    }
	}
	/* packet = parsed and in ascii in shared region -> copy to clients */
	snprintf(stringptr, maxstringlen, "\n");
	stringptr += strlen(stringptr);
	shared_setlen((stringptr - stringbuf));
	debug("Churnbuffer used: %d", (stringptr - stringbuf));
	master_permitread();
    }
    /* NOT REACHED */
    return (EX_SOFTWARE);
}
