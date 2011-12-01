/* gf128mul.c - GF(2^128) multiplication functions
 *
 * Copyright (c) 2003, Dr Brian Gladman, Worcester, UK.
 * Copyright (c) 2006, Rik Snel <rsnel@cube.dyndns.org>
 * Copyright (c) 2007, ntldr <ntldr@freed0m.org> PGP key ID - 0xC48251EB4F8E4E6E 
 *
 * Based on Dr Brian Gladman's (GPL'd) work published at
 * http://fp.gladman.plus.com/cryptography_technology/index.htm
 * See the original copyright notice below.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
*/
#include "defines.h"
#include "gf128mul.h"

#ifndef SMALL_CODE

#define gf128mul_dat(q) { \
         q(0x00), q(0x01), q(0x02), q(0x03), q(0x04), q(0x05), q(0x06), q(0x07),\
         q(0x08), q(0x09), q(0x0a), q(0x0b), q(0x0c), q(0x0d), q(0x0e), q(0x0f),\
         q(0x10), q(0x11), q(0x12), q(0x13), q(0x14), q(0x15), q(0x16), q(0x17),\
         q(0x18), q(0x19), q(0x1a), q(0x1b), q(0x1c), q(0x1d), q(0x1e), q(0x1f),\
         q(0x20), q(0x21), q(0x22), q(0x23), q(0x24), q(0x25), q(0x26), q(0x27),\
         q(0x28), q(0x29), q(0x2a), q(0x2b), q(0x2c), q(0x2d), q(0x2e), q(0x2f),\
         q(0x30), q(0x31), q(0x32), q(0x33), q(0x34), q(0x35), q(0x36), q(0x37),\
         q(0x38), q(0x39), q(0x3a), q(0x3b), q(0x3c), q(0x3d), q(0x3e), q(0x3f),\
         q(0x40), q(0x41), q(0x42), q(0x43), q(0x44), q(0x45), q(0x46), q(0x47),\
         q(0x48), q(0x49), q(0x4a), q(0x4b), q(0x4c), q(0x4d), q(0x4e), q(0x4f),\
         q(0x50), q(0x51), q(0x52), q(0x53), q(0x54), q(0x55), q(0x56), q(0x57),\
         q(0x58), q(0x59), q(0x5a), q(0x5b), q(0x5c), q(0x5d), q(0x5e), q(0x5f),\
         q(0x60), q(0x61), q(0x62), q(0x63), q(0x64), q(0x65), q(0x66), q(0x67),\
         q(0x68), q(0x69), q(0x6a), q(0x6b), q(0x6c), q(0x6d), q(0x6e), q(0x6f),\
         q(0x70), q(0x71), q(0x72), q(0x73), q(0x74), q(0x75), q(0x76), q(0x77),\
         q(0x78), q(0x79), q(0x7a), q(0x7b), q(0x7c), q(0x7d), q(0x7e), q(0x7f),\
         q(0x80), q(0x81), q(0x82), q(0x83), q(0x84), q(0x85), q(0x86), q(0x87),\
         q(0x88), q(0x89), q(0x8a), q(0x8b), q(0x8c), q(0x8d), q(0x8e), q(0x8f),\
         q(0x90), q(0x91), q(0x92), q(0x93), q(0x94), q(0x95), q(0x96), q(0x97),\
         q(0x98), q(0x99), q(0x9a), q(0x9b), q(0x9c), q(0x9d), q(0x9e), q(0x9f),\
         q(0xa0), q(0xa1), q(0xa2), q(0xa3), q(0xa4), q(0xa5), q(0xa6), q(0xa7),\
         q(0xa8), q(0xa9), q(0xaa), q(0xab), q(0xac), q(0xad), q(0xae), q(0xaf),\
         q(0xb0), q(0xb1), q(0xb2), q(0xb3), q(0xb4), q(0xb5), q(0xb6), q(0xb7),\
         q(0xb8), q(0xb9), q(0xba), q(0xbb), q(0xbc), q(0xbd), q(0xbe), q(0xbf),\
         q(0xc0), q(0xc1), q(0xc2), q(0xc3), q(0xc4), q(0xc5), q(0xc6), q(0xc7),\
         q(0xc8), q(0xc9), q(0xca), q(0xcb), q(0xcc), q(0xcd), q(0xce), q(0xcf),\
         q(0xd0), q(0xd1), q(0xd2), q(0xd3), q(0xd4), q(0xd5), q(0xd6), q(0xd7),\
         q(0xd8), q(0xd9), q(0xda), q(0xdb), q(0xdc), q(0xdd), q(0xde), q(0xdf),\
         q(0xe0), q(0xe1), q(0xe2), q(0xe3), q(0xe4), q(0xe5), q(0xe6), q(0xe7),\
         q(0xe8), q(0xe9), q(0xea), q(0xeb), q(0xec), q(0xed), q(0xee), q(0xef),\
         q(0xf0), q(0xf1), q(0xf2), q(0xf3), q(0xf4), q(0xf5), q(0xf6), q(0xf7),\
         q(0xf8), q(0xf9), q(0xfa), q(0xfb), q(0xfc), q(0xfd), q(0xfe), q(0xff) \
 }
 
 /*  Given the value i in 0..255 as the byte overflow when a field element
     in GHASH is multipled by x^8, this function will return the values that
     are generated in the lo 16-bit word of the field value by applying the
     modular polynomial. The values lo_byte and hi_byte are returned via the
     macro xp_fun(lo_byte, hi_byte) so that the values can be assembled into
     memory as required by a suitable definition of this macro operating on
     the table above
 */
 
#define xx(p, q)        0x##p##q

#define xda_bbe(i) ( \
        (i & 0x80 ? xx(43, 80) : 0) ^ (i & 0x40 ? xx(21, c0) : 0) ^ \
        (i & 0x20 ? xx(10, e0) : 0) ^ (i & 0x10 ? xx(08, 70) : 0) ^ \
        (i & 0x08 ? xx(04, 38) : 0) ^ (i & 0x04 ? xx(02, 1c) : 0) ^ \
        (i & 0x02 ? xx(01, 0e) : 0) ^ (i & 0x01 ? xx(00, 87) : 0) \
)

#define be128_xor(x,y,z) { \
	(x)->a = (y)->a ^ (z)->a; \
	(x)->b = (y)->b ^ (z)->b; \
  }

static const u16 gf128mul_table_bbe[256] = gf128mul_dat(xda_bbe);

static void gf128mul_x_bbe(be128 *r, const be128 *x)
{
	u64 a = BE64(x->a);
	u64 b = BE64(x->b);
	u64 t = gf128mul_table_bbe[a >> 63];

	r->a = BE64((a << 1) | (b >> 63));
    r->b = BE64((b << 1) ^ t);
}

void gf128mul_x_ble(be128 *r, const be128 *x)
{
	u64 a = x->a;
	u64 b = x->b;
	u64 _tt = gf128mul_table_bbe[b >> 63];

	r->a = (a << 1) ^ _tt;
	r->b = (b << 1) | (a >> 63);
}
 
static void gf128mul_x8_bbe(be128 *x)
{
	u64 a = BE64(x->a);
	u64 b = BE64(x->b);
	u64 t = gf128mul_table_bbe[a >> 56];

	x->a = BE64((a << 8) | (b >> 56));
	x->b = BE64((b << 8) ^ t);
}


 
void gf128mul_bbe(be128 *r, const be128 *b)
{
	be128 p[8];
	u8    ch;
    int   i;

	p[0] = *r;

	for (i = 0; i < 7; ++i) {
		gf128mul_x_bbe(&p[i + 1], &p[i]);
	}

	r->a = 0; r->b = 0;

	for (i = 0;;) 
	{
		ch = p8(b)[i];
 
		if (ch & 0x80) {
			be128_xor(r, r, &p[7]);
		}
		if (ch & 0x40) {
			be128_xor(r, r, &p[6]);
		}
		if (ch & 0x20) {
			be128_xor(r, r, &p[5]);
		}
		if (ch & 0x10) {
			be128_xor(r, r, &p[4]);
		}
		if (ch & 0x08) {
			be128_xor(r, r, &p[3]);
		}
		if (ch & 0x04) {
			be128_xor(r, r, &p[2]);
		}
		if (ch & 0x02) {
			be128_xor(r, r, &p[1]);
		}
		if (ch & 0x01) {
			be128_xor(r, r, &p[0]);
		}

		if (++i >= 16) {
			break;
		}

		gf128mul_x8_bbe(r);
	}
}

void gf128mul_init_32k(gf128mul_32k *ctx, const be128 *g)
{
	int i, j, k;
 
	zeroauto(ctx, sizeof(gf128mul_32k));

	ctx->t[0][1] = *g;

	for (j = 1; j <= 64; j <<= 1) {
		gf128mul_x_bbe(&ctx->t[0][j + j], &ctx->t[0][j]);
	}

	for (i = 0;;) 
	{
		for (j = 2; j < 256; j += j) {
			for (k = 1; k < j; ++k) {
				be128_xor(&ctx->t[i][j + k], 
					&ctx->t[i][j], &ctx->t[i][k]);
			}
		}

		if (++i >= 8) {
			break;
		}

		for (j = 128; j > 0; j >>= 1) {
			ctx->t[i][j] = ctx->t[i - 1][j];
			gf128mul_x8_bbe(&ctx->t[i][j]);
		}
	}
}

#ifndef GFMUL_ASM

/* multiplicate p = a*t in GF(128)
   "a" value may be stored in little-endian format 
*/
void gf128mul64_table(be128 *p, u8 *a, gf128mul_32k *ctx)
{
	be128 t = ctx->t[0][a[0]];

	xor128(&t, &t, &ctx->t[1][a[1]]);
	xor128(&t, &t, &ctx->t[2][a[2]]);
	xor128(&t, &t, &ctx->t[3][a[3]]);
	xor128(&t, &t, &ctx->t[4][a[4]]);
	xor128(&t, &t, &ctx->t[5][a[5]]);
	xor128(&t, &t, &ctx->t[6][a[6]]);	
	xor128(&t, &t, &ctx->t[7][a[7]]);	 

	*p = t;
}

#endif /* GFMUL_ASM */
#endif /* SMALL_CODE */