/*
 * $Id: symux.c,v 1.1 2001/08/20 14:40:12 dijkstra Exp $
 *
 * Daemon that multiplexes incoming mon traffic to subscribed clients
 * and archives it periodically in rrd files.
 */
#include <sys/types.h>
#include <machine/endian.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "readconf.h"

int listen_sock;
fd_set fdset;
int maxfd;
int signal_seen;

void signalhandler(s) 
    int s;
{
    signal_seen = s;
}

void close_listensock() 
{
    close(listen_sock);
}

int main(argc, argv)
    int argc;
    char *argv[];
{
    struct sockaddr_in servaddr;
    int ret;

    /* parse configuration file */
    read_config_file("/home/dijkstra/project/monmux/monmux.conf");

    /* catch signals */
//    signal(SIGALRM, alarmhandler);
    signal(SIGINT, signalhandler);
    signal(SIGQUIT, signalhandler);
    signal(SIGTERM, signalhandler);
    signal(SIGXCPU, signalhandler);
    signal(SIGXFSZ, signalhandler);
    signal(SIGUSR1, signalhandler);
    signal(SIGUSR2, signalhandler);

    /* setup listener */
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
	errx(1, "socket: %.200s", strerror(errno));
    } else {
	atexit( close_listensock );
    }
    memset((void *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(4000);
    if (bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
	errx(1,"Bind failed: %.200s.", strerror(errno));
    }
    if (listen(listen_sock,5)) {
	errx(1,"Listen failed: %.200s.", strerror(errno));
    }
    FD_ZERO (&fdset);
    FD_SET (listen_sock, &fdset);
    maxfd = 1;

    /*
     * Stay listening for connections until the daemon is killed with a signal 
     */
    for (;;) {
	ret = select(maxfd, &fdset, NULL, NULL, NULL);
	
	if (ret == -1) {
	    if (errno == EINTR) {
		/* Got an interrupt, bail out now :) */
	    }
	}

	if (ret == 0) {
	    /* Timer expired */
	}

//	if (ret == number) {
	/* Data on a number of fds *///	}
    }
}




