/* Regression test for shared memory information transfer between symux master
 * and n clients.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "error.h"
#include "share.h"

int verbose;

int
main(int argc, char *argv[])
{
    int i;
    int slot;
    int clients = 25;
    char *shm;
    struct timespec ts;

    ts.tv_sec = 0;
    ts.tv_nsec = 10000;

    /* test initialization */
    initshare(40960);

    flag_debug = 1;
    /* test forking n clients */
    for (i = 0; i < clients; i++)
        spawn_client(STDOUT_FILENO);

    /* throughput test */
    for (i = 0; i < 5000; i++) {
        slot = master_forbidread();
        shm = (char *)shared_getmem(slot);
        memset(&shm[0], (i % 26) + 65, 80);
        shared_setlen(slot, 80);
        master_permitread();
        info("step %d\n", i);

        nanosleep(&ts, NULL);
    }
    /* kill some clients */

    exit(1);
}
