/*
 * Copyright (c) 2021, Ilya Kurdyukov
 * All rights reserved.
 *
 * crzy64: An easy to decode base64 modification. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>

#ifndef CRZY64_ATTR
#define CRZY64_ATTR
#endif

#ifndef CRZY64_FAST64
#if defined(__LP64__) || defined(__x86_64__) || defined(__aarch64__) || defined(__e2k__)
#define CRZY64_FAST64 1
#else
#define CRZY64_FAST64 0
#endif
#endif

#ifndef CRZY64_UNALIGNED
#if defined(__i386__) || defined(__x86_64__) || defined(__aarch64__)
#define CRZY64_UNALIGNED 1
#else
#define CRZY64_UNALIGNED 0
#endif
#endif

#ifndef CRZY64_VEC
#define CRZY64_VEC 1
#endif

#ifndef CRZY64_NEON
#if defined(__ARM_NEON__) || defined(__aarch64__)
#define CRZY64_NEON 1
#else
#define CRZY64_NEON 0
#endif
#endif

#if CRZY64_VEC
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __SSSE3__
#include <tmmintrin.h>
#endif
#if CRZY64_NEON
#include <arm_neon.h>
#endif
#endif

/* 24 -> 6x4 */
static inline uint32_t crzy64_unpack(uint32_t a) {
	uint32_t ah, al, b;
	ah = a & 0xfcf0c0;
	al = a & 0x030f3f;
	b  = ah <<  6 ^ al;
	b ^= ah << 12 ^ al >> 6;
	b ^= ah << 18 ^ al >> 12;
	return b & 0x3f3f3f3f;
}

#if CRZY64_FAST64
/* 24x2 -> 6x8 */
static inline uint64_t crzy64_unpack64(uint64_t a) {
	// return crzy64_unpack(a) | (uint64_t)crzy64_unpack(a >> 32) << 32;
	uint64_t h, l, m = ~0xff000000llu;
	h = a & 0xfcf0c000fcf0c0;
	l = a & 0x030f3f00030f3f;
	h = (((h << 6 & m) ^ h) << 6 & m) ^ h;
	l = (((l >> 6 & m) ^ l) >> 6 & m) ^ l;
	return (l ^ h << 6) & 0x3f3f3f3f3f3f3f3f;
}
#endif

#define CRZY64_DEC1(a) ((((a) >> 5) + (a) + (((a) - 9) & -((a) >> 6))) & 63)

#define R4(x) (x * 0x01010101)
#define R8(x) (x * 0x0101010101010101)
#define CRZY64_ENC(R) do { \
	a = (a + R(5)) & R(63); \
	b = a & R(1); \
	c = R(127) + (((a + R(12)) >> 6) & R(1)); \
	a += (((b << 5) + R(71) - ((a + b) >> 1)) & c) - R(6); \
} while (0)

CRZY64_ATTR
size_t crzy64_encode(uint8_t *d, const uint8_t *s, size_t n) {
	uint8_t *d0 = d; uint32_t a, b, c;

#if CRZY64_VEC && defined(__SSE2__)
	if (n >= 12) {
		__m128i c1 = _mm_set1_epi8(1), c5 = _mm_set1_epi8(5);
		__m128i c52 = _mm_set1_epi8(52), c71 = _mm_set1_epi8(71);
		__m128i c63 = _mm_set1_epi8(63), a, b, c;
#ifdef __SSSE3__
		__m128i idx = _mm_setr_epi8(0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1);
#endif
		__m128i mh = _mm_set1_epi32(0xfcf0c0);
		__m128i ml = _mm_set1_epi32(0x030f3f);
		do {
#ifdef __SSSE3__
			a = _mm_loadl_epi64((const __m128i*)s);
			b = _mm_cvtsi32_si128(*(uint32_t*)(s + 8));
			a = _mm_unpacklo_epi64(a, b);
			a = _mm_shuffle_epi8(a, idx);
#else
			b = _mm_cvtsi32_si128(*(uint32_t*)s);
			c = _mm_cvtsi32_si128(*(uint32_t*)(s + 3));
			a = _mm_unpacklo_epi32(b, c);
			b = _mm_cvtsi32_si128(*(uint32_t*)(s + 6));
			c = _mm_cvtsi32_si128(*(uint32_t*)(s + 8));
			b = _mm_unpacklo_epi32(b, _mm_srli_epi32(c, 8));
			a = _mm_unpacklo_epi64(a, b);
#endif
			/* unpack */
			c = _mm_and_si128(a, mh);
			b = _mm_xor_si128(c, _mm_slli_epi32(c, 6));
			b = _mm_xor_si128(b, _mm_slli_epi32(c, 12));
			c = _mm_and_si128(a, ml);
			a = _mm_xor_si128(c, _mm_srli_epi32(c, 6));
			a = _mm_xor_si128(a, _mm_srli_epi32(c, 12));
			a = _mm_xor_si128(a, _mm_slli_epi32(b, 6));
			/* core */
			a = _mm_and_si128(_mm_add_epi8(a, c5), c63);
			b = _mm_and_si128(a, c1);
			c = _mm_add_epi8(_mm_slli_epi16(b, 5), c71);
			c = _mm_sub_epi8(c, _mm_srli_epi16(_mm_add_epi8(a, b), 1));
			c = _mm_and_si128(c, _mm_cmpgt_epi8(c52, a));
			a = _mm_sub_epi8(a, _mm_set1_epi8(6));
			a = _mm_add_epi8(a, c);
			_mm_storeu_si128((__m128i*)d, a);
			s += 12; n -= 12; d += 16;
		} while (n >= 12);
	}
#endif

#if CRZY64_FAST64
	if (n >= 6) do {
		uint64_t a, b, c;
#if CRZY64_UNALIGNED
		a = *(uint32_t*)s | (uint64_t)*(uint32_t*)(s + 2) << 24;
#else
		a = s[0] | s[1] << 8 | s[2] << 16;
		a |= (uint64_t)(s[3] | s[4] << 8 | s[5] << 16) << 32;
#endif
		a = crzy64_unpack64(a);
		CRZY64_ENC(R8);
#if CRZY64_UNALIGNED
		*(uint64_t*)d = a;
#else
		d[0] = a;
		d[1] = a >> 8;
		d[2] = a >> 16;
		d[3] = a >> 24;
		a >>= 32;
		d[4] = a;
		d[5] = a >> 8;
		d[6] = a >> 16;
		d[7] = a >> 24;
#endif
		s += 6; n -= 6; d += 8;
	} while (n >= 6);
#endif

	if (n >= 3) do {
		a = s[0] | s[1] << 8 | s[2] << 16;
		a = crzy64_unpack(a);
		CRZY64_ENC(R4);
#if CRZY64_UNALIGNED
		*(uint32_t*)d = a;
#else
		d[0] = a;
		d[1] = a >> 8;
		d[2] = a >> 16;
		d[3] = a >> 24;
#endif
		s += 3; n -= 3; d += 4;
	} while (n >= 3);

	if (n) {
		a = s[0];
		if (n > 1) a |= s[1] << 8;
		a = crzy64_unpack(a);
		CRZY64_ENC(R4);
		d[0] = a;
		d[1] = a >> 8;
		if (n > 1) d[2] = a >> 16;
		d += n + 1;
	}

	return d - d0;
}

#define CRZY64_DEC(a, R) (((((a) >> 5) & R(3)) + \
	(a) + (((a) - R(9)) & (R(64) - (((a) >> 6) & R(1))))) & R(63))
#define CRZY64_DEC4(a) CRZY64_DEC(a, R4)
#define CRZY64_DEC8(a) CRZY64_DEC(a, R8)
#define CRZY64_PACK(a) ((a) ^ (a) >> 6)

CRZY64_ATTR
size_t crzy64_decode(uint8_t *d, const uint8_t *s, size_t n) {
	uint8_t *d0 = d;
#if CRZY64_VEC && CRZY64_NEON
	if (n >= 16) {
		uint8x16_t c9 = vdupq_n_u8(9), c63 = vdupq_n_u8(63), a, b;
		union {
			uint32x4_t a; uint8x8x2_t b;
			uint8x16_t c; uint64x2_t d;
		} c;
		uint8x8_t idx0 = vcreate_u8(0x0908060504020100);
		uint8x8_t idx1 = vcreate_u8(0x0e0d0c0a);
#ifdef __aarch64__
		uint8x16_t idx = vcombine_u8(idx0, idx1);
#endif
		do {
			a = vld1q_u8(s);
			b = vandq_u8(vsubq_u8(a, c9), vcleq_u8(c63, a));
			a = vaddq_u8(vaddq_u8(a, vshrq_n_u8(a, 5)), b);
			a = vandq_u8(a, c63);
			c.a = vreinterpretq_u32_u8(a);
			c.a = veorq_u32(c.a, vshrq_n_u32(c.a, 6));
#ifdef __aarch64__
			c.c = vqtbl1q_u8(c.c, idx);
			vst1q_lane_u64((uint64_t*)d, c.d, 0);
			vst1q_lane_u32((uint32_t*)d + 2, c.a, 2);
#else
			vst1_u8(d, vtbl2_u8(c.b, idx0));
			vst1_lane_u32((uint32_t*)d + 2,
					vreinterpret_u32_u8(vtbl2_u8(c.b, idx1)), 0);
#endif
			s += 16; n -= 16; d += 12;
		} while (n >= 16);
	}
#elif CRZY64_VEC && defined(__SSE2__)
	if (n >= 16) {
		__m128i c3 = _mm_set1_epi8(3), c9 = _mm_set1_epi8(9);
		__m128i c63 = _mm_set1_epi8(63), a, b, c;
#ifdef __SSSE3__
		__m128i idx = _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1);
#else
		__m128i mask = _mm_set1_epi32(0xffffff);
#endif
		do {
			a = _mm_loadu_si128((const __m128i*)s);
			b = _mm_and_si128(_mm_srli_epi16(a, 5), c3);
			c = _mm_and_si128(_mm_sub_epi8(a, c9), _mm_cmpgt_epi8(a, c63));
			a = _mm_add_epi8(_mm_add_epi8(a, b), c);
			a = _mm_and_si128(a, c63);
			a = _mm_xor_si128(a, _mm_srli_epi32(a, 6));
#ifdef __SSSE3__
			a = _mm_shuffle_epi8(a, idx);
			_mm_storel_epi64((__m128i*)d, a);
			a = _mm_bsrli_si128(a, 8);
			*(uint32_t*)(d + 8) = _mm_cvtsi128_si32(a);
#else
			a = _mm_and_si128(a, mask);
			{
				uint32_t x, y;
				x = _mm_cvtsi128_si32(a); a = _mm_bsrli_si128(a, 4);
				y = _mm_cvtsi128_si32(a); a = _mm_bsrli_si128(a, 4);
				*(uint32_t*)d = x | y << 24;
				x = _mm_cvtsi128_si32(a); a = _mm_bsrli_si128(a, 4);
				*(uint32_t*)(d + 4) = y >> 8 | x << 16;
				y = _mm_cvtsi128_si32(a);
				*(uint32_t*)(d + 8) = y << 8 | x >> 16;
			}
#endif
			s += 16; n -= 16; d += 12;
		} while (n >= 16);
	}
#endif
#if CRZY64_FAST64
#if 0 // CRZY64_UNALIGNED && !CRZY64_VEC
	if (n >= 16) do {
		uint64_t a = *(uint64_t*)s, t;
		a = CRZY64_DEC8(a);
		a = CRZY64_PACK(a);
		*(uint32_t*)d = ((uint32_t)a & 0xffffff)
				| ((uint32_t)(a >> 8) & ~0xffffff);
		t = a >> 40 & 0xffff;
		a = *(uint64_t*)(s + 8);
		a = CRZY64_DEC8(a);
		a = CRZY64_PACK(a);
		*(uint32_t*)(d + 4) = a << 16 | t;
		*(uint32_t*)(d + 8) = ((uint32_t)(a >> 24) & ~0xff)
				| ((uint32_t)a >> 16 & 0xff);
		s += 16; n -= 16; d += 12;
	} while (n >= 16);
#endif

	if (n >= 8) do {
#if CRZY64_UNALIGNED
		uint64_t a = *(uint64_t*)s;
#else
		uint64_t a = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
		a |= (uint64_t)(s[4] | s[5] << 8 | s[6] << 16 | s[7] << 24) << 32;
#endif
		a = CRZY64_DEC8(a);
		a = CRZY64_PACK(a);
#if CRZY64_UNALIGNED
		*(uint32_t*)d = ((uint32_t)a & 0xffffff)
				| ((uint32_t)(a >> 8) & ~0xffffff);
		*(uint16_t*)(d + 4) = a >> 40;
#else
		d[0] = a;
		d[1] = a >> 8;
		d[2] = a >> 16;
		a >>= 32;
		d[3] = a;
		d[4] = a >> 8;
		d[5] = a >> 16;
#endif
		s += 8; n -= 8; d += 6;
	} while (n >= 8);

	if (n > 5) {
		uint64_t a = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
		a |= (uint64_t)(s[4] | s[5] << 8) << 32;
		if (n > 6) a |= (uint64_t)s[6] << 48;
		a = CRZY64_DEC8(a);
		a = CRZY64_PACK(a);
		d[0] = a;
		d[1] = a >> 8;
		d[2] = a >> 16;
		d[3] = a >> 32;
		if (n > 6) d[4] = a >> 40;
		d += n - 2;
	} else if (n > 1) {
		uint32_t a = s[0] | s[1] << 8;
		if (n > 2) a |= s[2] << 16;
		if (n > 3) a |= s[3] << 24;
		a = CRZY64_DEC4(a);
		a = CRZY64_PACK(a);
		d[0] = a;
		if (n > 2) d[1] = a >> 8;
		if (n > 3) d[2] = a >> 16;
		d += n - 1;
	}
#else
	if (n >= 4) do {
#if CRZY64_UNALIGNED
		uint32_t a = *(uint32_t*)s;
#else
		uint32_t a = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
#endif
		a = CRZY64_DEC4(a);
		a = CRZY64_PACK(a);
#if CRZY64_UNALIGNED
		*(uint16_t*)d = a;
		d[2] = a >> 16;
#else
		d[0] = a;
		d[1] = a >> 8;
		d[2] = a >> 16;
#endif
		s += 4; n -= 4; d += 3;
	} while (n >= 4);

	if (n > 1) {
		uint32_t a = s[0] | s[1] << 8;
		if (n > 2) a |= s[2] << 16;
		a = CRZY64_DEC4(a);
		a = CRZY64_PACK(a);
		d[0] = a;
		if (n > 2) d[1] = a >> 8;
		d += n - 1;
	}
#endif

	return d - d0;
}

