/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/* GLOBAL.H - RSAREF types and constants */

#include <string.h>
#include <stdint.h>
#include "../game/q_shared.h"
#include "qcommon.h"


#ifdef WIN32

#pragma warning(disable : 4711) // selected for automatic inline expansion

#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;


/* MD4.H - header file for MD4C.C */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991.

All rights reserved.

License to copy and use this software is granted provided that it is identified as the RSA Data Security, Inc. MD4 Message-Digest Algorithm in all material mentioning or referencing this software or this function.
License is also granted to make and use derivative works provided that such works are identified as derived from the RSA Data Security, Inc. MD4 Message-Digest Algorithm in all material mentioning or referencing the derived work.
RSA Data Security, Inc. makes no representations concerning either the merchantability of this software or the suitability of this software for any particular purpose. It is provided as is without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this documentation and/or software. */

/* MD4 context. */
typedef struct {
	UINT4 state[4];             /* state (ABCD) */
	UINT4 count[2];             /* number of bits, modulo 2^64 (lsb first) */
	unsigned char buffer[64];           /* input buffer */
} MD4_CTX;

void MD4Init( MD4_CTX * );
void MD4Update( MD4_CTX *, const unsigned char *, unsigned int );
void MD4Final( unsigned char [16], MD4_CTX * );


/* MD4C.C - RSA Data Security, Inc., MD4 message-digest algorithm */
/* Copyright (C) 1990-2, RSA Data Security, Inc. All rights reserved.

License to copy and use this software is granted provided that it is identified as the
RSA Data Security, Inc. MD4 Message-Digest Algorithm
 in all material mentioning or referencing this software or this function.
License is also granted to make and use derivative works provided that such works are identified as
derived from the RSA Data Security, Inc. MD4 Message-Digest Algorithm
in all material mentioning or referencing the derived work.
RSA Data Security, Inc. makes no representations concerning either the merchantability of this software or the suitability of this software for any particular purpose. It is provided
as is without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this documentation and/or software. */

/* Constants for MD4Transform routine.  */
#define S11 3
#define S12 7
#define S13 11
#define S14 19
#define S21 3
#define S22 5
#define S23 9
#define S24 13
#define S31 3
#define S32 9
#define S33 11
#define S34 15

static void MD4Transform( UINT4 [4], const unsigned char [64] );
static void Encode( unsigned char *, UINT4 *, unsigned int );
static void Decode( UINT4 *, const unsigned char *, unsigned int );

static unsigned char PADDING[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* F, G and H are basic MD4 functions. */
#define F( x, y, z ) ( ( ( x ) & ( y ) ) | ( ( ~x ) & ( z ) ) )
#define G( x, y, z ) ( ( ( x ) & ( y ) ) | ( ( x ) & ( z ) ) | ( ( y ) & ( z ) ) )
#define H( x, y, z ) ( ( x ) ^ ( y ) ^ ( z ) )

/* ROTATE_LEFT rotates x left n bits. */
#define ROTATE_LEFT( x, n ) ( ( ( x ) << ( n ) ) | ( ( x ) >> ( 32 - ( n ) ) ) )

/* FF, GG and HH are transformations for rounds 1, 2 and 3 */
/* Rotation is separate from addition to prevent recomputation */
#define FF( a, b, c, d, x, s ) {( a ) += F( ( b ), ( c ), ( d ) ) + ( x ); ( a ) = ROTATE_LEFT( ( a ), ( s ) );}

#define GG( a, b, c, d, x, s ) {( a ) += G( ( b ), ( c ), ( d ) ) + ( x ) + (UINT4)0x5a827999; ( a ) = ROTATE_LEFT( ( a ), ( s ) );}

#define HH( a, b, c, d, x, s ) {( a ) += H( ( b ), ( c ), ( d ) ) + ( x ) + (UINT4)0x6ed9eba1; ( a ) = ROTATE_LEFT( ( a ), ( s ) );}


/* MD4 initialization. Begins an MD4 operation, writing a new context. */
void MD4Init( MD4_CTX *context ) {
	context->count[0] = context->count[1] = 0;

/* Load magic initialization constants.*/
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

/* MD4 block update operation. Continues an MD4 message-digest operation, processing another message block, and updating the context. */
void MD4Update( MD4_CTX *context, const unsigned char *input, unsigned int inputLen ) {
	unsigned int i, index, partLen;

	/* Compute number of bytes mod 64 */
	index = ( unsigned int )( ( context->count[0] >> 3 ) & 0x3F );

	/* Update number of bits */
	if ( ( context->count[0] += ( (UINT4)inputLen << 3 ) ) < ( (UINT4)inputLen << 3 ) ) {
		context->count[1]++;
	}

	context->count[1] += ( (UINT4)inputLen >> 29 );

	partLen = 64 - index;

	/* Transform as many times as possible.*/
	if ( inputLen >= partLen ) {
		Com_Memcpy( (POINTER)&context->buffer[index], (POINTER)input, partLen );
		MD4Transform( context->state, context->buffer );

		for ( i = partLen; i + 63 < inputLen; i += 64 )
			MD4Transform( context->state, &input[i] );

		index = 0;
	} else {
		i = 0;
	}

	/* Buffer remaining input */
	Com_Memcpy( (POINTER)&context->buffer[index], (POINTER)&input[i], inputLen - i );
}


/* MD4 finalization. Ends an MD4 message-digest operation, writing the the message digest and zeroizing the context. */
void MD4Final( unsigned char digest[16], MD4_CTX *context ) {
	unsigned char bits[8];
	unsigned int index, padLen;

	/* Save number of bits */
	Encode( bits, context->count, 8 );

	/* Pad out to 56 mod 64.*/
	index = ( unsigned int )( ( context->count[0] >> 3 ) & 0x3f );
	padLen = ( index < 56 ) ? ( 56 - index ) : ( 120 - index );
	MD4Update( context, PADDING, padLen );

	/* Append length (before padding) */
	MD4Update( context, bits, 8 );

	/* Store state in digest */
	Encode( digest, context->state, 16 );

	/* Zeroize sensitive information.*/
	Com_Memset( (POINTER)context, 0, sizeof( *context ) );
}


/* MD4 basic transformation. Transforms state based on block. */
static void MD4Transform( UINT4 state[4], const unsigned char block[64] ) {
	UINT4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	Decode( x, block, 64 );

/* Round 1 */
	FF( a, b, c, d, x[ 0], S11 );           /* 1 */
	FF( d, a, b, c, x[ 1], S12 );           /* 2 */
	FF( c, d, a, b, x[ 2], S13 );           /* 3 */
	FF( b, c, d, a, x[ 3], S14 );           /* 4 */
	FF( a, b, c, d, x[ 4], S11 );           /* 5 */
	FF( d, a, b, c, x[ 5], S12 );           /* 6 */
	FF( c, d, a, b, x[ 6], S13 );           /* 7 */
	FF( b, c, d, a, x[ 7], S14 );           /* 8 */
	FF( a, b, c, d, x[ 8], S11 );           /* 9 */
	FF( d, a, b, c, x[ 9], S12 );           /* 10 */
	FF( c, d, a, b, x[10], S13 );       /* 11 */
	FF( b, c, d, a, x[11], S14 );       /* 12 */
	FF( a, b, c, d, x[12], S11 );       /* 13 */
	FF( d, a, b, c, x[13], S12 );       /* 14 */
	FF( c, d, a, b, x[14], S13 );       /* 15 */
	FF( b, c, d, a, x[15], S14 );       /* 16 */

/* Round 2 */
	GG( a, b, c, d, x[ 0], S21 );       /* 17 */
	GG( d, a, b, c, x[ 4], S22 );       /* 18 */
	GG( c, d, a, b, x[ 8], S23 );       /* 19 */
	GG( b, c, d, a, x[12], S24 );       /* 20 */
	GG( a, b, c, d, x[ 1], S21 );       /* 21 */
	GG( d, a, b, c, x[ 5], S22 );       /* 22 */
	GG( c, d, a, b, x[ 9], S23 );       /* 23 */
	GG( b, c, d, a, x[13], S24 );       /* 24 */
	GG( a, b, c, d, x[ 2], S21 );       /* 25 */
	GG( d, a, b, c, x[ 6], S22 );       /* 26 */
	GG( c, d, a, b, x[10], S23 );       /* 27 */
	GG( b, c, d, a, x[14], S24 );       /* 28 */
	GG( a, b, c, d, x[ 3], S21 );       /* 29 */
	GG( d, a, b, c, x[ 7], S22 );       /* 30 */
	GG( c, d, a, b, x[11], S23 );       /* 31 */
	GG( b, c, d, a, x[15], S24 );       /* 32 */

/* Round 3 */
	HH( a, b, c, d, x[ 0], S31 );           /* 33 */
	HH( d, a, b, c, x[ 8], S32 );       /* 34 */
	HH( c, d, a, b, x[ 4], S33 );       /* 35 */
	HH( b, c, d, a, x[12], S34 );       /* 36 */
	HH( a, b, c, d, x[ 2], S31 );       /* 37 */
	HH( d, a, b, c, x[10], S32 );       /* 38 */
	HH( c, d, a, b, x[ 6], S33 );       /* 39 */
	HH( b, c, d, a, x[14], S34 );       /* 40 */
	HH( a, b, c, d, x[ 1], S31 );       /* 41 */
	HH( d, a, b, c, x[ 9], S32 );       /* 42 */
	HH( c, d, a, b, x[ 5], S33 );       /* 43 */
	HH( b, c, d, a, x[13], S34 );       /* 44 */
	HH( a, b, c, d, x[ 3], S31 );       /* 45 */
	HH( d, a, b, c, x[11], S32 );       /* 46 */
	HH( c, d, a, b, x[ 7], S33 );       /* 47 */
	HH( b, c, d, a, x[15], S34 );       /* 48 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	/* Zeroize sensitive information.*/
	Com_Memset( (POINTER)x, 0, sizeof( x ) );
}


/* Encodes input (UINT4) into output (unsigned char). Assumes len is a multiple of 4. */
static void Encode( unsigned char *output, UINT4 *input, unsigned int len ) {
	unsigned int i, j;

	for ( i = 0, j = 0; j < len; i++, j += 4 ) {
		output[j] = ( unsigned char )( input[i] & 0xff );
		output[j + 1] = ( unsigned char )( ( input[i] >> 8 ) & 0xff );
		output[j + 2] = ( unsigned char )( ( input[i] >> 16 ) & 0xff );
		output[j + 3] = ( unsigned char )( ( input[i] >> 24 ) & 0xff );
	}
}


/* Decodes input (unsigned char) into output (UINT4). Assumes len is a multiple of 4. */
static void Decode( UINT4 *output, const unsigned char *input, unsigned int len ) {
	unsigned int i, j;

	for ( i = 0, j = 0; j < len; i++, j += 4 )
		output[i] = ( (UINT4)input[j] ) | ( ( (UINT4)input[j + 1] ) << 8 ) | ( ( (UINT4)input[j + 2] ) << 16 ) | ( ( (UINT4)input[j + 3] ) << 24 );
}

//===================================================================

unsigned Com_BlockChecksum( const void *buffer, int length ) {
	int digest[4];
	unsigned val;
	MD4_CTX ctx;

	MD4Init( &ctx );
	MD4Update( &ctx, (unsigned char *)buffer, length );
	MD4Final( (unsigned char *)digest, &ctx );

	val = digest[0] ^ digest[1] ^ digest[2] ^ digest[3];

	return val;
}

unsigned Com_BlockChecksumKey( void *buffer, int length, int key ) {
	int digest[4];
	unsigned val;
	MD4_CTX ctx;

	MD4Init( &ctx );
	MD4Update( &ctx, (unsigned char *)&key, 4 );
	MD4Update( &ctx, (unsigned char *)buffer, length );
	MD4Final( (unsigned char *)digest, &ctx );

	val = digest[0] ^ digest[1] ^ digest[2] ^ digest[3];

	return val;
}


/*
* This code implements the MD5 message-digest algorithm.
* The algorithm is due to Ron Rivest.  This code was
* written by Colin Plumb in 1993, no copyright is claimed.
* This code is in the public domain; do with it what you wish.
*
* Equivalent code is available from RSA Data Security, Inc.
* This code has been tested against that, and is equivalent,
* except that you don't need to include two pages of legalese
* with every copy.
*
* To compute the message digest of a chunk of bytes, declare an
* MD5Context structure, pass it to MD5Init, call MD5Update as
* needed on buffers full of bytes, and then call MD5Final, which
* will fill a supplied 16-byte array with the digest.
*/



#ifndef Q3_BIG_ENDIAN
#define byteReverse(buf, len)	/* Nothing */
#else
static void byteReverse(unsigned char *buf, unsigned longs);

/*
* Note: this code is harmless on little-endian machines.
*/
static void byteReverse(unsigned char *buf, unsigned longs)
{
	uint32_t t;
	do {
		t = (uint32_t)
			((unsigned)buf[3] << 8 | buf[2]) << 16 |
			((unsigned)buf[1] << 8 | buf[0]);
		*(uint32_t *)buf = t;
		buf += 4;
	} while (--longs);
}
#endif // Q3_BIG_ENDIAN

/*
* Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
* initialization constants.
*/
static void MD5Init(struct MD5Context *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}
/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/*
* The core of the MD5 algorithm, this alters an existing MD5 hash to
* reflect the addition of 16 longwords of new data.  MD5Update blocks
* the data and converts bytes into longwords for this routine.
*/
static void MD5Transform(uint32_t buf[4],
	uint32_t const in[16])
{
	register uint32_t a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
	MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

/*
* Update context to reflect the concatenation of another buffer full
* of bytes.
*/
static void MD5Update(struct MD5Context *ctx, unsigned char const *buf,
	unsigned len)
{
	uint32_t t;

	/* Update bitcount */

	t = ctx->bits[0];
	if ((ctx->bits[0] = t + ((uint32_t)len << 3)) < t)
		ctx->bits[1]++;		/* Carry from low to high */
	ctx->bits[1] += len >> 29;

	t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

	/* Handle any leading odd-sized chunks */

	if (t) {
		unsigned char *p = (unsigned char *)ctx->in + t;

		t = 64 - t;
		if (len < t) {
			memcpy(p, buf, len);
			return;
		}
		memcpy(p, buf, t);
		byteReverse(ctx->in, 16);
		MD5Transform(ctx->buf, (uint32_t *)ctx->in);
		buf += t;
		len -= t;
	}
	/* Process data in 64-byte chunks */

	while (len >= 64) {
		memcpy(ctx->in, buf, 64);
		byteReverse(ctx->in, 16);
		MD5Transform(ctx->buf, (uint32_t *)ctx->in);
		buf += 64;
		len -= 64;
	}

	/* Handle any remaining bytes of data. */

	memcpy(ctx->in, buf, len);
}


/*
* Final wrapup - pad to 64-byte boundary with the bit pattern
* 1 0* (64-bit count of bits processed, MSB-first)
*/
static void MD5Final(struct MD5Context *ctx, unsigned char *digest)
{
	unsigned count;
	unsigned char *p;

	/* Compute number of bytes mod 64 */
	count = (ctx->bits[0] >> 3) & 0x3F;

	/* Set the first char of padding to 0x80.  This is safe since there is
	always at least one byte free */
	p = ctx->in + count;
	*p++ = 0x80;

	/* Bytes of padding needed to make 64 bytes */
	count = 64 - 1 - count;

	/* Pad out to 56 mod 64 */
	if (count < 8) {
		/* Two lots of padding:  Pad the first block to 64 bytes */
		memset(p, 0, count);
		byteReverse(ctx->in, 16);
		MD5Transform(ctx->buf, (uint32_t *)ctx->in);

		/* Now fill the next block with 56 bytes */
		memset(ctx->in, 0, 56);
	}
	else {
		/* Pad block to 56 bytes */
		memset(p, 0, count - 8);
	}
	byteReverse(ctx->in, 14);

	/* Append length in bits and transform */
	((uint32_t *)ctx->in)[14] = ctx->bits[0];
	((uint32_t *)ctx->in)[15] = ctx->bits[1];

	MD5Transform(ctx->buf, (uint32_t *)ctx->in);
	byteReverse((unsigned char *)ctx->buf, 4);

	if (digest != NULL)
		memcpy(digest, ctx->buf, 16);
	memset(ctx, 0, sizeof(*ctx));	/* In case it's sensitive */
}

// L0 - Was missing
extern int FS_Read2(void *buffer, int len, fileHandle_t f);
// End

char *Com_MD5File(const char *fn, int length, const char *prefix, int prefix_len)
{
	static char final[33] = { "" };
	unsigned char digest[16] = { "" };
	fileHandle_t f;
	MD5_CTX md5;
	byte buffer[2048];
	int i;
	int filelen = 0;
	int r = 0;
	int total = 0;

	Q_strncpyz(final, "", sizeof(final));

	filelen = FS_SV_FOpenFileRead(fn, &f);

	if (!f) {
		return final;
	}
	if (filelen < 1) {
		FS_FCloseFile(f);
		return final;
	}
	if (filelen < length || !length) {
		length = filelen;
	}

	MD5Init(&md5);

	if (prefix_len && *prefix)
		MD5Update(&md5, (unsigned char *)prefix, prefix_len);

	for (;;) {
		r = FS_Read2(buffer, sizeof(buffer), f);
		if (r < 1)
			break;
		if (r + total > length)
			r = length - total;
		total += r;
		MD5Update(&md5, buffer, r);
		if (r < sizeof(buffer) || total >= length)
			break;
	}
	FS_FCloseFile(f);
	MD5Final(&md5, digest);
	final[0] = '\0';
	for (i = 0; i < 16; i++) {
		Q_strcat(final, sizeof(final), va("%02X", digest[i]));
	}
	return final;
}

char *Com_MD5(const void *data, int length, const char *prefix, int prefix_len, int hexcase)
{
	static char final[33];
	unsigned char digest[16];
	MD5_CTX md5;
	int i;
	char *fmt;

	if (hexcase) {
		fmt = "%02X";
	}
	else {
		fmt = "%02x";
	}
	MD5Init(&md5);
	if (prefix_len && *prefix) {
		MD5Update(&md5, (unsigned char *)prefix, prefix_len);
	}
	MD5Update(&md5, (unsigned char *)data, length);
	MD5Final(&md5, digest);
	*final = 0;
	for (i = 0; i < 16; i++) {
		Q_strcat(final, sizeof(final), va(fmt, digest[i]));
	}
	return final;
}
