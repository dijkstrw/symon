/*
 * $Id: data.h,v 1.4 2002/03/29 16:29:19 dijkstra Exp $
 *
 * A host carrying a 'mon' is considered a 'source' of information. A single
 * data 'stream' of information has a particular type: <cpu|mem|if|io>. A
 * source can provide multiple 'streams' simultaniously. A source spools
 * information towards a 'mux'. A 'stream' that has been converted to network
 * representation is called a 'packedstream'.
 */
#ifndef _DATA_H
#define _DATA_H

#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

/* Mon stream version 
 * version 1:
 * <mon version:8><time in seconds since epoch:32><streams>*
 */
#define MON_STREAMVER  1

struct monpacket {
    struct {
	u_int8_t mon_version;
	u_int64_t timestamp;
	u_int16_t length;
	u_int16_t crc;
    } header;
    char data[_POSIX2_LINE_MAX];
};  
  
/* Note that packedstreams are never associated with streams directly. The
   streams are only used to see what should be done to the data in the
   packedstream. */
struct stream {
    int type;
    char *args;
    char *file;
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
    int socket;
    u_int32_t senderr;
    u_int32_t ip;
    u_int16_t port;
    struct sockaddr_in sockaddr;
    struct streamlist sl;
    struct monpacket packet;
    int offset;
    SLIST_ENTRY(mux) muxes;
};
SLIST_HEAD(muxlist, mux);

/* ps2str types */
#define PS2STR_PRETTY 0
#define PS2STR_RRD    1

/* Stream types */
#define MT_IO     0
#define MT_CPU    1
#define MT_MEM    2
#define MT_IF     3
#define MT_EOT    4

/* NOTE: struct packetstream
 *
 * Unpacking of incoming packets is done via a packedstream structure. This
 * structure defines the maximum amount of data that can be contained in a
 * single network representation of a stream. 
 *
 * This sucks in the way of portability (e.g. adding multiple cpu types) and
 * maintainabilty. This code will be refactored when I port it to other oses. 
 */
#define MON_PS_ARGLEN    16
struct packedstream {
    int type;
    int padding;
    char args[MON_PS_ARGLEN];
    union {
	struct { 
	    u_int64_t mtotal_transfers;
	    u_int64_t mtotal_seeks;
	    u_int64_t mtotal_bytes;
	} ps_io;
	struct {
	    u_int16_t muser;
	    u_int16_t mnice;
	    u_int16_t msystem;
	    u_int16_t minterrupt;
	    u_int16_t midle;
	} ps_cpu;
	struct {
	    u_int32_t mreal_active; 
	    u_int32_t mreal_total;
	    u_int32_t mfree;
	    u_int32_t mswap_used;
	    u_int32_t mswap_total;
	} ps_mem;
	struct {
	    u_int32_t mipackets;
	    u_int32_t mopackets;
	    u_int32_t mibytes; 
	    u_int32_t mobytes;
	    u_int32_t mimcasts; 
	    u_int32_t momcasts;
	    u_int32_t mierrors; 
	    u_int32_t moerrors;
	    u_int32_t mcolls;
	    u_int32_t mdrops;
	} ps_if;
    } data;
};

#define mio_total_transfers  data.ps_io.mtotal_transfers
#define mio_total_seeks      data.ps_io.mtotal_seeks
#define mio_total_bytes      data.ps_io.mtotal_bytes
#define mcpm_user            data.ps_cpu.uuser
#define mcpm_nice            data.ps_cpu.unice
#define mcpm_system          data.ps_cpu.usystem
#define mcpm_interrupt       data.ps_cpu.uinterrupt
#define mcpm_idle            data.ps_cpu.uidle
#define mmem_real_active     data.ps_mem.mreal_active
#define mmem_real_total      data.ps_mem.mreal_total
#define mmem_free            data.ps_mem.mfree
#define mmem_swap_msed       data.ps_mem.uswap_used
#define mmem_swap_total      data.ps_mem.mswap_total
#define mif_ipackets         data.ps_if.mipackets
#define mif_opackets         data.ps_if.mopackets
#define mif_ibytes           data.ps_if.mibytes
#define mif_obytes           data.ps_if.mobytes
#define mif_imcasts          data.ps_if.mimcasts
#define mif_omcasts          data.ps_if.momcasts
#define mif_ierrors          data.ps_if.mierrors
#define mif_oerrors          data.ps_if.moerrors
#define mif_colls            data.ps_if.mcolls
#define mif_drops            data.ps_if.mdrops


/* prototypes */
__BEGIN_DECLS
int            token2type __P((int));
int            snpack __P((char *, int, char*, int, ...));
int            sunpack __P((char *, struct packedstream *));
int            psdata2strn __P((struct packedstream *, char *, int, int));
struct stream *find_source_stream __P((struct source *, int, char *));
struct stream *add_source_stream __P((struct source *, int, char *)); 
struct stream *find_mux_stream __P((struct mux *,  int, char *)); 
struct stream *add_mux_stream __P((struct mux *, int, char *)); 
struct source *find_source __P((struct sourcelist *, char *));
struct source *find_source_ip __P((struct sourcelist *, u_int32_t));
struct source *add_source __P((struct sourcelist *, char *));
struct mux    *find_mux __P((struct muxlist *, char *));
struct mux    *add_mux __P((struct muxlist *, char *));
__END_DECLS
#endif                                      /* _DATA_H */


