/* $Id: testshare.c,v 1.3 2004/08/08 19:56:03 dijkstra Exp $ */

/* Regression test for shared memory information transfer between symux master
 * and n clients.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "conf.h"
#include "share.h"

int verbose;

int
main(int argc, char *argv[])
{
    int i;
    int j;
    int clients = 25;
    char *shm;

/* test initialization */
    initshare();

    shm = (char *) shared_getmem();
/* test forking n clients */
    for (i=0; i<clients; i++)
	spawn_client(0);

/* throughput test */
    for (i=50000; i > 0; i--) {
	master_forbidread();
	for (j=0; j<clients; j++)
	    printf(".");
	memset(&shm[2], (i%26)+65, 80);
	shm[1] = 80;
	master_permitread();
	usleep(1);
    }
/* kill some clients */

    exit(1);

}
