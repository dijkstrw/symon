/*
 * Copyright (c) 2001-2024 Willem Dijkstra
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

#include "conf.h"

/* Share contains all routines needed for the ipc between symuxes */
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <sys/mman.h>

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "data.h"
#include "error.h"
#include "net.h"
#include "share.h"
#include "symux.h"
#include "symuxnet.h"

/* Shared operation:
 *
 * The moment the master symux receives a new packet:
 *
 * master determines the dataslot and issues master_forbidread for that slot:
 * - checks if the read semaphore for the slot is zero. If not, reap_clients
 *   is called to reap any stopped clients
 * - adds any pending new clients
 * - reset the semaphore for the slot to prevent clients to start reading
 *
 * master copies new data into the dataslot and calls master_permitread
 * - increment sequence number in the shared region
 * - increment read semaphore for the slot with the number of registered
 *   clients
 *
 * clients call client_waitread:
 * - increment client sequence number and determine dataslot
 * - exit if client sequence number < (shared sequence - max dataslots - 1)
 * - block on semaphore for dataslot
 *
 * clients read the data in the shared region
 *
 */

__BEGIN_DECLS
void check_master(void);
void check_sem(void);
void client_doneread(void);
void client_loop(void);
void client_signalhandler(int);
int client_waitread(void);
void exitmaster(void);
void master_resetsem(int);
void reap_clients(void);
__END_DECLS

int realclients; /* number of clients active */
int newclients;
int master;     /* is current process master or child */
int clientsock; /* connected client */
pid_t clientpid;

enum ipcstat { SIPC_FREE, SIPC_KEYED, SIPC_ATTACHED };
struct sharedregion *shm = MAP_FAILED;
int shm_size = 0;
key_t semid;
enum ipcstat semstat;
int seqnr;

/* Get start of data in shared region */
char *
shared_getmem(int slot)
{
    long offset = ((slot % SYMUX_SHARESLOTS) * shm->slotlen);
    char *data = (char *)&shm->data + offset;

    return data;
}

/* Get max length of data stored in shared region */
long
shared_getmaxlen(void)
{
    return shm->slotlen;
}

/* Set length of data stored in shared region */
void
shared_setlen(int slot, long length)
{
    if (length > shm->slotlen)
        fatal("%s:%d: internal error:"
              "set_length of shared region called with value larger "
              "than actual size",
            __FILE__, __LINE__);

    shm->ctlen[slot % SYMUX_SHARESLOTS] = length;
}

/* Get length of data stored in shared region */
long
shared_getlen(int slot)
{
    return shm->ctlen[slot % SYMUX_SHARESLOTS];
}

/* Check whether semaphore is available */
void
check_sem(void)
{
    if (semstat != SIPC_KEYED)
        fatal("%s:%d: internal error: semaphore not available", __FILE__,
            __LINE__);
}

/* Check whether process is the master process */
void
check_master(void)
{
    if (master == 0)
        fatal("%s:%d: internal error: child process tried to access "
              "master routines",
            __FILE__, __LINE__);
}

/* Reset semaphores before each distribution cycle */
void
master_resetsem(int semnum)
{
    union semun semarg;

    semarg.val = 0;

    check_sem();
    check_master();

    if ((semctl(semid, semnum, SETVAL, semarg) != 0))
        fatal("%s:%d: internal error: cannot reset semaphore %d", __FILE__,
            __LINE__, semnum);
}

/* Prepare for writing to shm */
int
master_forbidread(void)
{
    int slot = (shm->seqnr + 1) % SYMUX_SHARESLOTS;
    int stalledclients;
    union semun semarg;

    check_sem();
    check_master();

    /* prepare for a new read */
    semarg.val = 0;
    if ((stalledclients = semctl(semid, slot, GETVAL, semarg)) < 0) {
        fatal("%s:%d: internal error: cannot read semaphore", __FILE__,
            __LINE__);
    } else {
        reap_clients();
        debug("realclients = %d; stalledclients = %d", realclients,
            stalledclients);
    }

    /* add new clients */
    realclients += newclients;
    newclients = 0;
    master_resetsem(slot);

    return slot;
}

/* Signal 'permit read' to all clients */
void
master_permitread(void)
{
    int slot = ++shm->seqnr % SYMUX_SHARESLOTS;
    union semun semarg;

    semarg.val = realclients;

    if (semctl(semid, slot, SETVAL, semarg) != 0)
        fatal("%s:%d: internal error: cannot set semaphore %d", __FILE__,
            __LINE__, slot);
}

/* Make clients wait until master signals */
int
client_waitread(void)
{
    int slot = ++seqnr % SYMUX_SHARESLOTS;
    struct sembuf sops;

    if (seqnr < (shm->seqnr - SYMUX_SHARESLOTS - 1)) {
        close(clientsock);
        fatal("%s:%d: client(%d) lagging behind (%d, %d) = high "
              "load?",
            __FILE__, __LINE__, clientpid, seqnr, shm->seqnr);
    }

    check_sem();

    sops.sem_num = slot;
    sops.sem_op = -1;
    sops.sem_flg = 0;

    if (semop(semid, &sops, 1) != 0)
        fatal("%s:%d: internal error: client(%d): cannot obtain "
              "semaphore (%.200s)",
            __FILE__, __LINE__, clientpid, strerror(errno));

    return slot;
}

/* Client signal 'done reading' to master */
void
client_doneread(void)
{
    /* force scheduling by sleeping a single second */
    sleep(1);
}

/* Client signal handler => always exit */
void
client_signalhandler(int s)
{
    debug("client(%d) received signal %d - quitting", clientpid, s);
    exit(EX_TEMPFAIL);
}

/* Prepare sharing structures for use */
void
initshare(int bufsize)
{
    int i;

    newclients = 0;
    realclients = 0;
    master = 1;

    /* need some extra space for housekeeping */
    shm_size = (bufsize * SYMUX_SHARESLOTS) + sizeof(struct sharedregion);

    /* allocate shared memory region for control information */
    semstat = SIPC_FREE;

    atexit(exitmaster);

    if ((shm = mmap(NULL, shm_size, PROT_READ | PROT_WRITE,
             MAP_ANONYMOUS | MAP_SHARED, -1, 0))
        == MAP_FAILED) {
        fatal("could not get shared memory region");
    }

    bzero(shm, shm_size);
    debug("shm from 0x%8x to 0x%8x", shm, shm + shm_size);
    shm->slotlen = bufsize;

    /* allocate semaphores */
    if ((semid = semget(IPC_PRIVATE, SYMUX_SHARESLOTS, SEM_ARGS)) < 0) {
        fatal("could not get a semaphore");
    }

    semstat = SIPC_KEYED;

    for (i = 0; i < SYMUX_SHARESLOTS; i++) {
        master_resetsem(i);
    }
}

/* Spawn off a new client */
pid_t
spawn_client(int sock)
{
    pid_t pid;

    check_master();

    clientsock = sock;

    if ((pid = fork())) {
        /* server */
        if (pid == -1) {
            info("could not fork client process");
        } else {
            newclients++;
            info("forked client(%d) for incoming connection from "
                 "%.200s:%.200s",
                pid, res_host, res_service);
        }

        if (clientsock > STDERR_FILENO) {
            close(clientsock);
        }
        return pid;
    } else {
        /* client */
        master = 0;
        seqnr = shm->seqnr;

        /* catch signals */
        signal(SIGHUP, client_signalhandler);
        signal(SIGINT, client_signalhandler);
        signal(SIGQUIT, client_signalhandler);
        signal(SIGTERM, client_signalhandler);
        signal(SIGTERM, client_signalhandler);
        signal(SIGPIPE, client_signalhandler);

        clientpid = getpid();
        client_loop();

        /* NOT REACHED */
        return 0;
    }
}
/* Reap exit/stopped clients */
void
reap_clients(void)
{
    pid_t pid;
    int status;

    status = 0;

    /* Reap all children that died */
    while (((int)(pid = wait4(-1, &status, WNOHANG, NULL)) > 0)
        && realclients >= 0) {
        /*
         * wait4 is supposed to return 0 if there is no status to
         * report, but it also reports -1 on OpenBSD 2.9
         */

        if (WIFEXITED(status))
            info("client process %d exited", pid, status);
        if (WIFSIGNALED(status))
            info("client process %d killed with signal %d", pid,
                WTERMSIG(status));
        if (WIFSTOPPED(status))
            info("client process %d stopped with signal %d", pid,
                WSTOPSIG(status));

        realclients--;
    }
}
/* Remove shared memory and semaphores at exit */
void
exitmaster(void)
{
    union semun semarg;

    if (master == 0)
        return;

    if (shm != MAP_FAILED) {
        if (munmap(shm, shm_size) == -1) {
            warning("%s:%d: internal error: control region could "
                    "not be detached",
                __FILE__, __LINE__);
        } else {
            shm = MAP_FAILED;
            shm_size = 0;
        }
    }

    switch (semstat) {
    case SIPC_KEYED:
        semarg.val = 0;
        if (semctl(semid, 0, IPC_RMID, semarg) != 0)
            warning("%s:%d: internal error: could not remove "
                    "semaphore %d",
                __FILE__, __LINE__, semid);
        /* FALLTHROUGH */
    case SIPC_FREE:
        break;

    default:
        warning("%s:%d: internal error: semaphore is in an unknown "
                "state",
            __FILE__, __LINE__);
    }
}
void
client_loop(void)
{
    int slot;
    int total;
    int sent;
    int written;

    for (;;) { /* FOREVER */

        slot = client_waitread();
        total = shared_getlen(slot);
        sent = 0;

        while (sent < total) {
            if ((written = write(clientsock,
                     (char *)(shared_getmem(slot) + sent), total - sent))
                == -1) {
                info("client(%d): write error. Client will "
                     "quit.",
                    clientpid);
                exit(1);
            }

            sent += written;

            debug("client(%d): written %d bytes of %d total from "
                  "slot %d",
                clientpid, sent, total, slot);
        }
    }
}
