/* $Id: npack.c,v 1.2 2003/10/10 15:19:58 dijkstra Exp $ */

/* Regression test to test marshalling and demarshalling
 *
 * Test one - For each datatype pack 4 values (zero value, maximum value,
 * minimal value, normal value) into a network buffer.
 */
#include <assert.h>
#include <string.h>

#include "xmalloc.h"
#include "data.h"

int main(int argc, char **argv) 
{
    char *buffer;
    char *str;
    struct packedstream *ps;
    int ilen, olen;

    buffer = xmalloc(_POSIX2_LINE_MAX);
    str = xmalloc(_POSIX2_LINE_MAX);
    ps = xmalloc(sizeof(struct packedstream));

    ilen = snpack(buffer, _POSIX2_LINE_MAX, "test pack", MT_TEST, 
		  /* test L, u_int64_t */
		  (u_int64_t) 0, (u_int64_t) 0xffffffffffffffff, (u_int64_t) 0, (u_int64_t) 0xffffff, 
		  /* test D, double */
		  (double) 0, (double) 100000, (double) -100000, (double) -12.05,
		  /* test l, u_int32_t */
		  (u_int32_t) 0, (u_int32_t) 0xffffffff, (u_int32_t) 0, (u_int32_t) 0x12345678,
		  /* test s, u_int16_t */
		  (int) 0, (int) 0xffff, (int) 0, (int) 0x8765,
		  /* test c, float */
		  (double) 0.0 , (double) 100.0, (double) 0, (double) 12.34,
		  /* test b, u_int8_t */
		  (int) 0, (int) 0xff, (int) 0, (int) 0x12);
    olen = sunpack(buffer, ps);

/*    ps2strn(ps, str, _POSIX2_LINE_MAX, PS2STR_PRETTY);
      printf(str); */

    assert(ilen == olen);
    assert(strcmp("test pack", ps->args) == 0);
    assert(ps->data.ps_test.L[0] == 0);
    assert(ps->data.ps_test.L[1] == 0xffffffffffffffff);
    assert(ps->data.ps_test.L[2] == 0);
    assert(ps->data.ps_test.L[3] == 0xffffff);
    assert(ps->data.ps_test.D[0] == 0);
    assert(ps->data.ps_test.D[1] == 100000000000);
    assert(ps->data.ps_test.D[2] == -100000000000);
    assert(ps->data.ps_test.D[3] == -12050000);
    assert(ps->data.ps_test.l[0] == 0);
    assert(ps->data.ps_test.l[1] == 0xffffffff);
    assert(ps->data.ps_test.l[2] == 0);
    assert(ps->data.ps_test.l[3] == 0x12345678);
    assert(ps->data.ps_test.s[0] == 0);
    assert(ps->data.ps_test.s[1] == 0xffff);
    assert(ps->data.ps_test.s[2] == 0);
    assert(ps->data.ps_test.s[3] == 0x8765);
    assert(ps->data.ps_test.c[0] == 0);
    assert(ps->data.ps_test.c[1] == 10000);
    assert(ps->data.ps_test.c[2] == 0);
    assert(ps->data.ps_test.c[3] == 1234);
    assert(ps->data.ps_test.b[0] == 0);
    assert(ps->data.ps_test.b[1] == 0xff);
    assert(ps->data.ps_test.b[2] == 0);
    assert(ps->data.ps_test.b[3] == 0x12);
    
    return 0;
}
