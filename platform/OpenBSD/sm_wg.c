/*
 * Copyright (c) 2024 Tim Kuijsten
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    - Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Get the current WireGuard peer statistics from the kernel and return them in
 * symon_buf as
 *
 * total bytes received : total bytes transmitted : last handshake
 */

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_wg.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "conf.h"
#include "error.h"
#include "errno.h"
#include "symon.h"
#include "xmalloc.h"

static struct wg_data_io *wg_stats;
static size_t wg_stats_count;
static int sock = -1;

void
init_wg(struct stream *st)
{
	struct wg_data_io      *wgdata;
	char *peerdesc;
	size_t i;

	if (geteuid() != 0) {
		warning("%s requires symon to run as the superuser, use -u",
			st->arg);
		return;
	}

	/* split arg on interface and peer name */
	if (strlcpy(st->parg.wg.full, st->arg, sizeof(st->parg.wg.full))
	    >= sizeof(st->parg.wg.full))
		fatal("interface name plus peer description exceed %d bytes: %s",
		    sizeof(st->parg.wg.full), st->arg);

	peerdesc = strchr(st->arg, '-');
	if (peerdesc == NULL)
		fatal("could not find dash between interface name and peer description: %s",
		    st->arg);

	*peerdesc = '\0';
	peerdesc++;
	if (strlen(peerdesc) == 0)
		fatal("peer description empty: %s", st->parg.wg.full);

	if (strlen(peerdesc) > SYMON_WGPEERDESC)
		fatal("peer description exceeds %d characters: %s",
			SYMON_WGPEERDESC, peerdesc);

	st->parg.wg.peerdesc = peerdesc;

	if (sock == -1) {
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock == -1)
			fatal("socket");
	}

	wgdata = NULL;
	for (i = 0; i < wg_stats_count; i++) {
		if (strcmp(wg_stats[i].wgd_name, st->arg) == 0) {
			wgdata = &wg_stats[i];
			break;
		}
	}

	if (wgdata == NULL) {
		wg_stats = xreallocarray(wg_stats, wg_stats_count + 1,
		    sizeof(*wg_stats));
		wgdata = &wg_stats[wg_stats_count];
		strlcpy(wgdata->wgd_name, st->arg, sizeof(wgdata->wgd_name));
		wgdata->wgd_size = 0;
		wgdata->wgd_interface = NULL;
		wg_stats_count++;
	}

	info("started module wg(%.200s)", st->parg.wg.full);
}

void
gets_wg(void)
{
	struct wg_data_io *wgdata;
	struct wg_interface_io *p;
	size_t i, last_size;

	for (i = 0; i < wg_stats_count; i++) {
		wgdata = &wg_stats[i];
		for (;;) {
			last_size = wgdata->wgd_size;
			if (ioctl(sock, SIOCGWG, wgdata) == -1) {
				if (errno != ENXIO)
					warning("%s: SIOCGWG: %s",
					    wgdata->wgd_name, strerror(errno));
				break;
			}

			if (last_size >= wgdata->wgd_size)
				break;

			if (wgdata->wgd_size > SYMON_MAX_DOBJECTS) {
				warning("%s:%d: dynamic object limit (%d) "
				    "exceeded for wg_data_io structure",
				    __FILE__, __LINE__, SYMON_MAX_DOBJECTS);
				free(wgdata->wgd_interface);
				wgdata->wgd_interface = NULL;
				wgdata->wgd_size = 0;
				break;
			}

			p = realloc(wgdata->wgd_interface, wgdata->wgd_size);
			if (p == NULL) {
				warning("wg %s: could not allocate memory: %s",
				    wgdata->wgd_name, wgdata->wgd_size,
				    strerror(errno));
				free(wgdata->wgd_interface);
				wgdata->wgd_interface = NULL;
				wgdata->wgd_size = 0;
				break;
			}

			wgdata->wgd_interface = p;
		}
	}
}

int
get_wg(char *symon_buf, int maxlen, struct stream *st)
{
	struct wg_interface_io *wg_interface;
	struct wg_peer_io      *wg_peer;
	struct wg_aip_io       *wg_aip;
	size_t i;

	wg_interface = NULL;
	for (i = 0; i < wg_stats_count; i++) {
		if (strcmp(wg_stats[i].wgd_name, st->arg) == 0) {
			wg_interface = wg_stats[i].wgd_interface;
			break;
		}
	}

	if (wg_interface == NULL)
		return 0;

	wg_peer = &wg_interface->i_peers[0];
	for (i = 0; i < wg_interface->i_peers_count; i++) {
		if (strcmp(wg_peer->p_description, st->parg.wg.peerdesc) != 0) {
			wg_aip = &wg_peer->p_aips[0];
			wg_aip += wg_peer->p_aips_count;
			wg_peer = (struct wg_peer_io *)wg_aip;
			continue;
		}

		return snpack(symon_buf, maxlen, st->parg.wg.full, MT_WG,
		    wg_peer->p_rxbytes,
		    wg_peer->p_txbytes,
		    wg_peer->p_last_handshake.tv_sec);
	}

	debug("couldn't find peer with description \"%s\" on %s", st->parg.wg.peerdesc,
	    st->arg);

	return 0;
}
