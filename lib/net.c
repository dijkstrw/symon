/*
 * Copyright (c) 2001-2004 Willem Dijkstra
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
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <string.h>

#include "conf.h"
#include "data.h"
#include "error.h"
#include "net.h"

/*
 * getip( address | fqdn ) - get ip address
 *
 * getip returns 1 if address could be reworked into an ip address. Resolved
 * data is stored in the globals res_host. The address structure res_addr is
 * also filled with sockaddr information that was obtained.
 */
char res_host[NI_MAXHOST];
char res_service[NI_MAXSERV];
struct sockaddr_storage res_addr;
int
getip(char *name, int family)
{
    struct addrinfo hints, *res;
    int error;

    res = NULL;
    bzero((void *) &hints, sizeof(struct addrinfo));

    /* don't lookup if we have a numeric address already */
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = family;
    if (getaddrinfo(name, NULL, &hints, &res) != 0) {
        hints.ai_flags = 0;
        if ((error = getaddrinfo(name, NULL, &hints, &res)) < 0) {
            debug("getaddrinfo(%.200s, %d): %.200s", name, family, gai_strerror(error));
            return 0;
        }
    }

    if (res && hints.ai_flags & AI_NUMERICHOST) {
        strncpy(res_host, name, NI_MAXHOST);
        res_host[NI_MAXHOST - 1] = 0;

        cpysock(res->ai_addr, &res_addr);

        freeaddrinfo(res);
        return 1;
    } else {
        if (res && res->ai_addr) {
            if ((error = getnameinfo(res->ai_addr, res->ai_addrlen,
                                     res_host, NI_MAXHOST,
                                     NULL, 0, NI_NUMERICHOST)) == 0) {
                res_host[NI_MAXHOST - 1] = 0;

                cpysock(res->ai_addr, &res_addr);

                freeaddrinfo(res);
                return 1;
            } else {
                debug("getnameinfo(%.200s, %d): %.200s", name, family, gai_strerror(error));
            }
        } else {
            debug("getip(%.200s, family): could not get numeric host via getaddrinfo nor getnameinfo", name, family);
        }
    }

    return 0;
}
/*
 * getaddr( address | fqdn, service ) - get the addrinfo structure
 *
 * getaddr returns a sockaddr structure in res_addr. it will only resolve
 * the address if that is necessary.
 */
int
getaddr(char *name, char *service, int socktype, int flags)
{
    struct addrinfo hints, *res;
    int error;

    res = NULL;
    bzero((void *) &hints, sizeof(hints));

    hints.ai_flags = flags;
    hints.ai_socktype = socktype;

    /* don't lookup if not necessary */
    hints.ai_flags |= AI_NUMERICHOST;
    if (getaddrinfo(name, service, &hints, &res) != 0) {
        hints.ai_flags = flags;
        if ((error = getaddrinfo(name, service, &hints, &res)) < 0) {
            warning("getaddrinfo(%.200s): %.200s", name, gai_strerror(error));
            return 0;
        }
    }

    if (res->ai_addrlen > sizeof(res_addr))
        fatal("%s:%d: internal error: getaddr returned bigger sockaddr than expected (%d>%d)",
              __FILE__, __LINE__, res->ai_addrlen, sizeof(res_addr));

    cpysock(res->ai_addr, &res_addr);

    return 1;
}
int
get_numeric_name(struct sockaddr_storage * source)
{
    snprintf(res_host, sizeof(res_host), "<unknown>");
    snprintf(res_service, sizeof(res_service), "<unknown>");

    return getnameinfo((struct sockaddr *)source, SS_LEN(source),
                       res_host, sizeof(res_host),
                       res_service, sizeof(res_service),
                       NI_NUMERICHOST | NI_NUMERICSERV);
}
void
cpysock(struct sockaddr * source, struct sockaddr_storage * dest)
{
    bzero(dest, sizeof(struct sockaddr_storage));
    bcopy(source, dest, SA_LEN(source));
}
/*
 * cmpsock_addr(sockaddr, sockaddr)
 *
 * compare if two sockaddr are talking about the same host
 */
int
cmpsock_addr(struct sockaddr * first, struct sockaddr * second)
{

    if (first == NULL || second == NULL)
        return 0;

    if ((SA_LEN(first) != SA_LEN(second)) ||
        (first->sa_family != second->sa_family))
        return 0;

    if (first->sa_family == PF_INET) {
        if (bcmp((void *) &((struct sockaddr_in *) first)->sin_addr,
                 (void *) &((struct sockaddr_in *) second)->sin_addr,
                 sizeof(struct in_addr)) == 0)
            return 1;
        else
            return 0;
    }

    if (first->sa_family == PF_INET6) {
        if (bcmp((void *) &((struct sockaddr_in6 *) first)->sin6_addr,
                 (void *) &((struct sockaddr_in6 *) second)->sin6_addr,
                 sizeof(struct in6_addr)) == 0)
            return 1;
        else
            return 0;
    }

    /* do not know what to compare for this family */
    return 0;
}
/* generate sockaddr based on family, type and getaddrinfo flags  */
void
get_sockaddr(struct sockaddr_storage * sockaddr, int family, int socktype,
    int flags, char * host, char * port)
{
    struct addrinfo hints, *res;

    bzero((void *) &hints, sizeof(struct addrinfo));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_flags = flags;
    if (getaddrinfo(host, port, &hints, &res) != 0)
        fatal("could not get sockaddr address");
    else {
        cpysock((struct sockaddr *) res->ai_addr, sockaddr);
        freeaddrinfo(res);
    }
}
/* fill a source->sockaddr with a sockaddr for use in address compares */
int
get_source_sockaddr(struct source * source, int family)
{
    if (getip(source->addr, family)) {
        cpysock((struct sockaddr *) &res_addr, &source->sockaddr);
        return 1;
    }

    return 0;
}
/* fill mux->sockaddr with a udp listen sockaddr */
void
get_mux_sockaddr(struct mux * mux, int socktype)
{
    if (getaddr(mux->addr, mux->port, socktype, AI_PASSIVE) == 0)
        fatal("could not get address information for %.200s %.200s",
              mux->addr, mux->port);

    cpysock((struct sockaddr *) &res_addr, &mux->sockaddr);
}
