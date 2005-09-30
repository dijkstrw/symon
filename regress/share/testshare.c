/* $Id: testshare.c,v 1.4 2005/09/30 14:05:12 dijkstra Exp $ */

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

/* test forking n clients */
    for (i=0; i<clients; i++)
	spawn_client(0);

/* throughput test */
    for (i=5000; i > 0; i--) {
	shm = (char *) shared_getmem(i);
	master_forbidread();
	for (j=0; j<clients; j++)
	    printf(".");
	memset(&shm[2], (i%26)+65, 80);
	shm[1] = 80;
	master_permitread();
	info("step %d\n", i);
    }
/* kill some clients */

    exit(1);

}
