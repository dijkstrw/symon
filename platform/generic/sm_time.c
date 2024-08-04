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

#include <sys/types.h>

#define _XOPEN_SOURCE 600
#include <errno.h>
#include <string.h>
#include <time.h>

#include "error.h"
#include "symon.h"

static struct timespec prevtime, curtime;
static int initialized;

void
init_time(struct stream *st)
{
	if (initialized == 1)
		fatal("time module can be configured only once");

	initialized = 1;

	info("started module time()");
}

int
get_time(char *symon_buf, int maxlen, struct stream *st)
{
	uint32_t diff;

	prevtime = curtime;
	if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &curtime) == -1)
		fatal("time: clock_gettime: %s", strerror(errno));

	if (prevtime.tv_sec == 0 && prevtime.tv_nsec == 0)
		return 0; /* wait before we have a measurement */

	diff = curtime.tv_sec - prevtime.tv_sec;
	diff *= 1000000U;
	diff += (curtime.tv_nsec  + 500) / 1000;
	diff -= (prevtime.tv_nsec + 500) / 1000;

	return snpack(symon_buf, maxlen, st->arg, MT_TIME, diff);
}
