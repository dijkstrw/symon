/* $Id: npack.c,v 1.1 2003/10/05 07:52:37 dijkstra Exp $ */

/* Regression test to test marshalling and demarshalling
 *
 * Test one - For each datatype pack 4 values (zero value, maximum value,
 * minimal value, normal value) into a network buffer.
 */
#include <assert.h>
#include <string.h>

#include "data.h"

int main(int argc, char **argv) 
{
    char buffer[4][_POSIX2_LINE_MAX];
    struct packedstream ps[4];
    int ilen[4], olen[4];

    bzero(&buffer[0][0], sizeof(buffer));
    bzero(&buffer[1][0], sizeof(buffer));
    bzero(&buffer[2][0], sizeof(buffer));
    bzero(&buffer[3][0], sizeof(buffer));
    bzero(&ps[0], sizeof(ps));
    bzero(&ps[1], sizeof(ps));
    bzero(&ps[2], sizeof(ps));
    bzero(&ps[3], sizeof(ps));

    /* test L, u_int64_t */
    ilen[0] = snpack(&buffer[0][0], sizeof(buffer), "test L", MT_PF, 
		     (u_int64_t) 0, (u_int64_t) 0xffffffffffffffff, (u_int64_t) 0, (u_int64_t) 0xffffff, 
		     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    olen[0] = sunpack(&buffer[0][0], &ps[0]);

    assert(ilen[0] == olen[0]);
    assert(strcmp("test L", ps[0].args) == 0);
    assert(ps[0].data.ps_pf.bytes_v4_in == 0);
    assert(ps[0].data.ps_pf.bytes_v4_out == 0xffffffffffffffff);
    assert(ps[0].data.ps_pf.bytes_v6_in == 0);
    assert(ps[0].data.ps_pf.bytes_v6_out == 0xffffff);

    /* test D, int64_t */
    ilen[0] = snpack(&buffer[0][0], sizeof(buffer), "test D1", MT_SENSOR,
		     (int64_t) 0);
    ilen[1] = snpack(&buffer[1][0], sizeof(buffer), "test D2", MT_SENSOR,
		     (int64_t) 0x7fffffffffffffff);
    ilen[2] = snpack(&buffer[2][0], sizeof(buffer), "test D3", MT_SENSOR,
		     (int64_t) -1);
    ilen[3] = snpack(&buffer[3][0], sizeof(buffer), "test D4", MT_SENSOR,
		     (int64_t) 428030200203020);
    olen[0] = sunpack(&buffer[0][0], &ps[0]);
    olen[1] = sunpack(&buffer[1][0], &ps[1]);
    olen[2] = sunpack(&buffer[2][0], &ps[2]);
    olen[3] = sunpack(&buffer[3][0], &ps[3]);
    
    assert(ilen[0] == ilen[0]);
    assert(ilen[1] == ilen[1]);
    assert(ilen[2] == ilen[2]);
    assert(ilen[3] == ilen[3]);
    assert(strcmp("test D1", ps[0].args) == 0);
    assert(strcmp("test D2", ps[1].args) == 0);
    assert(strcmp("test D3", ps[2].args) == 0);
    assert(strcmp("test D4", ps[3].args) == 0);
    assert(ps[0].data.ps_sensor.value == 0);
    assert(ps[1].data.ps_sensor.value == 0x7fffffffffffffff);
    assert(ps[2].data.ps_sensor.value == -1);
    assert(ps[3].data.ps_sensor.value == 428030200203020);
    
    /* test l, u_int32_t */
    /* test s, u_int16_t */
    /* test c, u_int16_t, actual = float * 10 */
    /* test b, u_int8_t */
    return 0;
}
