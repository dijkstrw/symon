/* $Id: */

/*
 * Copyright (c) 2001-2002 Willem Dijkstra
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

/* TODO:
 * Dynamically allocate buffer size 
 * Check wether one buffer suffices, do some performance tests
 */

/* Share contains all routines needed for the ipc between monmuxes */
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "data.h"
#include "error.h"
#include "muxnet.h"
#include "share.h"

/* Shared operation: 
 *
 * The moment the master monmux receives a new packet:
 *
 * master calls master_forbidread:
 * - master checks the 'SEM_READ' semaphore to be equal to the number of
 *   children active. If so, all children are up to date. If not, a child is
 *   lagging and will die soon. Reap_clients().
 * - master adds any pending new clients
 * - master resets all semaphores, preventing clients to start reading
 *
 * master calls master_allowread:
 * - increment sequence number in the shared region
 * - increment 'SEM_WAIT' with the number of registered clients
 *
 * clients call client_waitread:
 * - block on semaphore 'SEM_WAIT'
 * - set client sequence number to shared region sequence number
 *
 * clients do something to the data in the shared region
 *
 * clients call client_doneread:
 * - If the recorded sequence number deviates from the one in the shared region
 *   the client is lagging behind. The client will kill itself.
 * - The client increments 'SEM_READ'.
 *
 */

#define SEM_WAIT     0              /* wait semaphore */
#define SEM_READ     1              /* have read semaphore */
#define SEM_TOTAL    2

int   realclients;               /* number of clients active */
int   newclients;
int   master;                    /* is current process master or child */
int   clientsock;                /* connected client */

enum  ipcstat { SIPC_FREE, SIPC_KEYED, SIPC_ATTACHED };
key_t shmid;
long  *shm;
enum ipcstat shmstat;
key_t semid;
enum ipcstat semstat;
int   seqnr;

/* Check whether semaphore is available */
void
check_sem()
{
    if (semstat != SIPC_KEYED)
	fatal("%s:%d: Internal error: Semaphore not available",
	      __FILE__, __LINE__);
}

/* Check whether process is the master process */
void
check_master()
{
    if (master == 0)
	fatal("%s:%d: Internal error: Child process tried to access master routines",
	      __FILE__, __LINE__);
}

/* Reset semaphores before each distribution cycle */
void 
master_resetsem()
{
    check_sem();
    check_master();

    if ((semctl(semid, SEM_WAIT, SETVAL, 0) != 0) ||
	(semctl(semid, SEM_READ, SETVAL, 0) != 0))
	fatal("%s:%d: Internal error: Cannot reset semaphores",
	      __FILE__, __LINE__);
}
/* Prepare for writing to shm */
void
master_forbidread()
{
    int clientsread;

    check_sem();
    check_master();
    
    /* prepare for a new read */
    if ((clientsread = semctl(semid, SEM_READ, GETVAL, 0)) < 0)
	fatal("%s:%d: Internal error: Cannot read semaphore",
	      __FILE__, __LINE__);

    if (clientsread != realclients)
	reap_clients();
    
    /* add new clients */
    realclients += newclients;
    newclients = 0;    
    master_resetsem();
}
/* Signal 'allow read' to all clients */
void
master_permitread()
{
    shm[0]++;

    if (semctl(semid, SEM_WAIT, SETVAL, realclients) != 0)
	fatal("%s:%d: Internal error: Cannot reset semaphores",
	      __FILE__, __LINE__);
}

/* Make clients wait until master signals */
void
client_waitread()
{
    struct sembuf sops;

    check_sem();

    sops.sem_num = SEM_WAIT;
    sops.sem_op  = -1;
    sops.sem_flg = 0;

    if (semop(semid, &sops, 1) != 0 )
	fatal("%s:%d: Internal error: Cannot obtain semaphore (%.200s)",
	      __FILE__, __LINE__, strerror(errno));

    seqnr = shm[0];
}

/* Client signal 'done reading' to master */
void
client_doneread()
{
    struct sembuf sops;

    check_sem();

    if (seqnr != shm[0])
	fatal("%s:%d: Internal error: Client lagging behind (%d, %d)",
	      __FILE__, __LINE__, seqnr, shm[0]);

    sops.sem_num = SEM_READ;
    sops.sem_op  = 1;
    sops.sem_flg = IPC_NOWAIT;

    if (semop(semid, &sops, 1) != 0 )
	fatal("%s:%d: Internal error: Cannot release semaphore (%.200s)",
	      __FILE__, __LINE__, strerror(errno));
}

/* Prepare sharing structures for use */
void 
initshare()
{
    newclients = 0;
    realclients = 0;
    master = 1;
    
    /* allocate shared memory region for control information */
    shmstat = semstat = SIPC_FREE;

    atexit(exitmaster);

    if ((shmid = shmget(IPC_PRIVATE, sizeof(long) * 4096, SHM_R | SHM_W)) < 0)
	fatal("Could not get a shared memory identifier");
    
    shmstat = SIPC_KEYED;

    if ((int)(shm = (long *)shmat(shmid, 0, 0)) < 0)
	fatal("Could not attach shared memory");

    shmstat = SIPC_ATTACHED;
    bzero(shm, (sizeof(long) * 4096));
    
    /* allocate semaphores */
    if ((semid = semget(IPC_PRIVATE, SEM_TOTAL, SEM_A | SEM_R)) < 0)
	fatal("Could not get a semaphore");
    
    semstat = SIPC_KEYED;

    master_resetsem();
}

/* Spawn off a new client */
void
spawn_client(int sock) 
{
    pid_t client_pid;

    check_master();

    if ((client_pid = fork())) {
	/* server */
	newclients++;
    } else {
	/* client */
	master = 0;
	clientsock = acceptconnection(sock);
	client_loop();
    }
}

/* Reap exit/stopped clients */
void
reap_clients()
{
    pid_t clientpid;
    int   status;
    
    status = 0;

    /* Reap all children that died */
    while (((int)(clientpid = wait4(-1, &status, WNOHANG, NULL)) != 0) && realclients >= 0) {
	if (WIFEXITED(status))
	    info("client process %d exited", status);
	if (WIFSIGNALED(status))
	    info("client process %d killed with signal %d", status, WTERMSIG(status));
	if (WIFSTOPPED(status))
	    info("client process %d stopped with signal %d", status, WSTOPSIG(status));

	realclients--;
    }
}

/* Remove shared memory and semaphores at exit */
void 
exitmaster() 
{
    if (master == 0)
	return;
	
    switch (shmstat) {
    case SIPC_ATTACHED:
	if (shmdt(shm))
	    warning("%s:%d: Internal error: control region could not be detached",
		    __FILE__, __LINE__);

	/* no break */
    case SIPC_KEYED:
	if (shmctl(shmid, IPC_RMID, NULL))
	    warning("%s:%d: Internal error: could remove control region %d",
		    __FILE__, __LINE__, shmid);
	/* no break */
    case SIPC_FREE:
	break;

    default:
	warning("%s:%d: Internal error: control region is in an unknown state",
		__FILE__, __LINE__);
    }

    switch (semstat) {
    case SIPC_KEYED:
	if (semctl(semid, 0, IPC_RMID, 0) != 0) 
	    warning("%s:%d: Internal error: could not remove semaphore %d",
		    __FILE__, __LINE__, semid);
	/* no break */
    case SIPC_FREE:
	break;

    default:
	warning("%s:%d: Internal error: semaphore is in an unknown state",
		__FILE__, __LINE__);
    }
}

/* Calculate maximum buffer space needed for a single mon hit */
int 
calculate_churnbuffer(struct sourcelist *sol) { 
    struct source *source; 
    struct stream *stream; 
    int maxlen;
    int len; 
    int n;

    len = n = 0; 
    source = NULL; 
    stream = NULL;
    /* determine maximum string size for a single source */
    SLIST_FOREACH(source, sol, sources) {
	maxlen = 0;
	SLIST_FOREACH(stream, &source->sl, streams) {
	    
	    len += strlentype(stream->type);
	    if (len > maxlen) maxlen = len;
	}
	debug("Source %s has %d sources and maxlen %d (=%d)", source->name, n, maxlen, n*maxlen);
    }
    return maxlen;
}

void 
client_loop()
{
    int total;
    int sent;

    for (;;) { /* FOREVER */
	
	client_waitread();
	
	total = shm[1];
	sent = 0;

	while (sent < total) {
	    sent += write(clientsock, &shm[2], total - sent);
	}

	client_doneread();
    }
}
