/*
 * $Id: data.h,v 1.1 2002/03/09 16:20:41 dijkstra Exp $
 *
 * A host carrying a 'mon' is considered a 'source' of information. A single
 * data 'stream' of information has a particular type: <cpu|mem|if|io>. A
 * source can provide multiple 'streams' simultaniously. A source spools
 * information towards a 'mux'.
 *
 * This library provides datastructures and routines to keep track of this
 * information.
 *
 */
#ifndef _DATA_H
#define _DATA_H

#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <limits.h>

#include "lex.h"

#ifdef WORDS_BIGENDIAN
#define htonq(n) n
#define ntohq(n) n
#else
static inline u_int64_t
htonq (u_int64_t v)
{
   return htonl ((u_int32_t) (v >> 32))
      | (u_int64_t) ntohl ((u_int32_t) v) << 32;
}
static inline u_int64_t
ntohq (u_int64_t v) 
{
   return ntohl ((u_int32_t) (v >>32))
      | (u_int64_t) ntohl ((u_int32_t) v) << 32;
}
#endif

/* Stream types */
#define MT_IO     0
#define MT_CPU    1
#define MT_MEM    2
#define MT_IF     3
#define MT_EOT    4

struct stream {
    int type;
    char *args;
    SLIST_ENTRY(stream) streams;
};
SLIST_HEAD(streamlist, stream);

struct source {
    char *name;
    u_int32_t ip;
    u_int16_t port;
    struct streamlist sl;
    SLIST_ENTRY(source) sources;
};
SLIST_HEAD(sourcelist, source);

struct mux {
    char *name;
    u_int32_t ip;
    u_int16_t port;
    struct streamlist sl;
    char data[_POSIX2_LINE_MAX];
    int offset;
    SLIST_ENTRY(mux) muxes;
};
SLIST_HEAD(muxlist, mux);

/* Unpacking of incoming packets is done via a packedstream structure. This
   structure defines the maximum amount of data that can be contained in a
   single network representation of a stream. My first version used variadric
   arguments, which makes for very simple exploits. 

   This sucks in the way of portability (e.g. adding multiple cpu types)
   and maintainabilty. I believe in extreme programming and refactoring only when
   there is a tangible need, so I'm going to use this code as is. 

   The structure is setup to be reused.  */

struct packedstream {
    int type;
    int padding;
    char args[_POSIX2_LINE_MAX];
    union {
	struct { 
	    u_int64_t utotal_transfers;
	    u_int64_t utotal_seeks;
	    u_int64_t utotal_bytes;
	} ps_io;
	struct {
	    u_int16_t uuser;
	    u_int16_t unice;
	    u_int16_t usystem;
	    u_int16_t uinterrupt;
	    u_int16_t uidle;
	} ps_cpu;
	struct {
	    u_int32_t ureal_active; 
	    u_int32_t ureal_total;
	    u_int32_t ufree;
	    u_int32_t uswap_used;
	    u_int32_t uswap_total;
	} ps_mem;
	struct {
	    u_int32_t uipackets;
	    u_int32_t uopackets;
	    u_int32_t uibytes; 
	    u_int32_t uobytes;
	    u_int32_t uimcasts; 
	    u_int32_t uomcasts;
	    u_int32_t uierrors; 
	    u_int32_t uoerrors;
	    u_int32_t ucolls;
	    u_int32_t udrops;
	} ps_if;
    } data;
};

#define io_total_transfers  data.ps_io.utotal_transfers
#define io_total_seeks      data.ps_io.utotal_seeks
#define io_total_bytes      data.ps_io.utotal_bytes
#define cpu_user            data.ps_cpu.uuser
#define cpu_nice            data.ps_cpu.unice
#define cpu_system          data.ps_cpu.usystem
#define cpu_interrupt       data.ps_cpu.uinterrupt
#define cpu_idle            data.ps_cpu.uidle
#define mem_real_active     data.ps_mem.ureal_active
#define mem_real_total      data.ps_mem.ureal_total
#define mem_free            data.ps_mem.ufree
#define mem_swap_used       data.ps_mem.uswap_used
#define mem_swap_total      data.ps_mem.uswap_total
#define if_ipackets         data.ps_if.uipackets
#define if_opackets         data.ps_if.uopackets
#define if_ibytes           data.ps_if.uibytes
#define if_obytes           data.ps_if.uobytes
#define if_imcasts          data.ps_if.uimcasts
#define if_omcasts          data.ps_if.uomcasts
#define if_ierrors          data.ps_if.uierrors
#define if_oerrors          data.ps_if.uoerrors
#define if_colls            data.ps_if.ucolls
#define if_drops            data.ps_if.udrops


/* prototypes */
__BEGIN_DECLS
int            token2type __P((int));
int            snpack __P((char *, int, char*, int, ...));
int            snunpack __P((char *, struct packedstream *));
struct stream *find_source_stream __P((struct source *, int, char *));
struct stream *add_source_stream __P((struct source *, int, char *)); 
struct stream *find_mux_stream __P((struct mux *,  int, char *)); 
struct stream *add_mux_stream __P((struct mux *, int, char *)); 
struct source *find_source __P((struct sourcelist *, char *));
struct source *add_source __P((struct sourcelist *, char *));
struct mux    *find_mux __P((struct muxlist *, char *));
struct mux    *add_mux __P((struct muxlist *, char *));
__END_DECLS
#endif                                      /* _DATA_H */


