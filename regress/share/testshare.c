/* $Id: */

/*
 * Test shared memory functions
 *
 */
#include <unistd.h>
#include "share.h"

int verbose;

int
main(int argc, char *argv[])
{
    int i;
    int j;
    int clients = 25;

/* test initialization */
    initshare();
/* test forking n clients */
    for (i=0; i<clients; i++)
	spawn_client();

/* throughput test */
    for (i=50000; i > 0; i--) {
	master_forbidread();
	for (j=0; j<clients; j++)
	    printf(".");
	memset(&shm[2], (i%26)+65, 80);
	shm[1] = 80;
	master_allowread();
	usleep(1);
    }
/* kill some clients */

    exit(1);

}
