/*
 * $Id: symux.c,v 1.3 2001/09/20 19:26:33 dijkstra Exp $
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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "error.h"
#include "monmux.h"
#include "readconf.h"
#include "limits.h"

struct hub *hub;
struct source *sources;

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
    struct sockaddr_in sins, sind;
    struct hostent *hp;
    char cbuf[_POSIX2_LINE_MAX];
    int error;
    socklen_t sl;

    /* parse configuration file */
    read_config_file("monmux.conf");

    /* catch signals */
//    signal(SIGALRM, alarmhandler);
    signal(SIGINT, signalhandler);
    signal(SIGQUIT, signalhandler);
    signal(SIGTERM, signalhandler);
    signal(SIGXCPU, signalhandler);
    signal(SIGXFSZ, signalhandler);
    signal(SIGUSR1, signalhandler);
    signal(SIGUSR2, signalhandler);

    memset(&sins, 0, sizeof(sins));
    sins.sin_family = AF_INET;
    sins.sin_port = htons(hub->port);
    hp = gethostbyname(hub->name);
    if (hp == NULL) {
	herror(hub->name);
	return (-1);
    }
    memcpy(&(sins.sin_addr.s_addr), hp->h_addr, hp->h_length);
    
    listen_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sock < 0) {
	fatal("socket: %.200s", strerror(errno));
    } else {
	atexit( close_listensock );
    }

    if (bind(listen_sock, (struct sockaddr *)&sins, sizeof(sins)) < 0) {
	fatal("Bind failed: %.200s.", strerror(errno));
    }

    for (;;) {
	sl = sizeof(sind);
	error = recvfrom(listen_sock, cbuf, sizeof(cbuf), 0, (struct sockaddr *)&sind, &sl);
	if ( error < 0 ) {
	    /* do sth */
	} else {
	    cbuf[error] = '\0';
//	    process(ntohl(sind.sin_addr.s_addr), cbuf);
	}
    }
    /* NOT REACHED */
}




