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

#ifndef CRZY64_H
#define CRZY64_H

#include <stdint.h>

#ifndef CRZY64_ATTR
#define CRZY64_ATTR
#endif

#ifndef CRZY64_FAST64
#if defined(__LP64__) || defined(__x86_64__) || defined(__aarch64__) \
		|| defined(__e2k__)
#define CRZY64_FAST64 1
#else
#define CRZY64_FAST64 0
#endif
#endif

#ifndef CRZY64_UNALIGNED
#if defined(__i386__) || defined(__x86_64__) || defined(__aarch64__) \
		|| (defined(__e2k__) && __iset__ >= 5)
#define CRZY64_UNALIGNED 1
#else
#define CRZY64_UNALIGNED 0
#endif
#endif

#if defined(__e2k__) && defined(__LCC__)
#define CRZY64_E2K_LCC
#endif

#ifndef CRZY64_VEC
#if !defined(CRZY64_E2K_LCC)
#define CRZY64_VEC 1
#else
#define CRZY64_VEC 0
#endif
#endif

#ifndef CRZY64_NEON
#if defined(__ARM_NEON__) || defined(__aarch64__)
#define CRZY64_NEON 1
#else
#define CRZY64_NEON 0
#endif
#endif

#ifndef CRZY64_RESTRICT
#define CRZY64_RESTRICT restrict
#endif

#ifndef CRZY64_PREFETCH
#ifdef __GNUC__
#define CRZY64_PREFETCH(p) __builtin_prefetch(p, 0)
#else
#define CRZY64_PREFETCH(p)
#endif
#endif

#if CRZY64_VEC
#if CRZY64_NEON
#include <arm_neon.h>
#elif defined(__AVX2__)
#include <immintrin.h>
#elif defined(__SSE4_1__)
#include <smmintrin.h>
#elif defined(__SSSE3__)
#include <tmmintrin.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#else
#undef CRZY64_VEC
#define CRZY64_VEC 0
#endif
#endif

#ifndef CRZY64_UNROLL
#if !defined(CRZY64_E2K_LCC)
#define CRZY64_UNROLL 2
#else
#define CRZY64_UNROLL 0
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

#define CRZY64_REP4(x) (x * 0x01010101)
#define CRZY64_REP8(x) (x * 0x0101010101010101)
#define CRZY64_ENC(R) do { \
	a = (a + R(5)) & R(63); \
	b = a & R(1); \
	c = R(127) + (((a + R(12)) >> 6) & R(1)); \
	a += (((b << 5) + R(71) - ((a + b) >> 1)) & c) - R(6); \
} while (0)

CRZY64_ATTR
size_t crzy64_encode(uint8_t *CRZY64_RESTRICT d,
		const uint8_t *CRZY64_RESTRICT s, size_t n) {
	uint8_t *d0 = d; uint32_t a, b, c;

#if CRZY64_VEC && CRZY64_NEON
	if (n >= 12) {
		uint8x16_t c5 = vdupq_n_u8(5), a, b, c;
		uint8x16_t c52 = vdupq_n_u8(52), c71 = vdupq_n_u8(71);
		uint8x16_t c63 = vdupq_n_u8(63), c6 = vdupq_n_u8(6);
		uint8x8_t idx0 = vcreate_u8(0xff050403ff020100);
#ifdef __aarch64__
		uint8x8_t idx1 = vcreate_u8(0xff0b0a09ff080706);
		uint8x16_t idx = vcombine_u8(idx0, idx1);
#else
		uint8x8_t idx1 = vcreate_u8(0xff070605ff040302);
#endif
		uint32x4_t ml = vdupq_n_u32(0x030f3f), x, y, z;
		do {
#ifdef __aarch64__
			a = vcombine_u8(vld1_u8(s),
					vcreate_u8(*(const uint32_t*)(s + 8)));
			a = vqtbl1q_u8(a, idx);
#else
			a = vcombine_u8(vtbl1_u8(vld1_u8(s), idx0),
					vtbl1_u8(vld1_u8(s + 4), idx1));
#endif
			/* unpack */
			x = vreinterpretq_u32_u8(a);
			z = vbicq_u32(x, ml);
			y = veorq_u32(z, vshlq_n_u32(z, 6));
			y = veorq_u32(y, vshlq_n_u32(z, 12));
			z = vandq_u32(x, ml);
			x = veorq_u32(z, vshrq_n_u32(z, 6));
			x = veorq_u32(x, vshrq_n_u32(z, 12));
			x = veorq_u32(x, vshlq_n_u32(y, 6));
			a = vreinterpretq_u8_u32(x);
			/* core */
			a = vandq_u8(vaddq_u8(a, c5), c63);
			b = vandq_u8(vshlq_n_u8(a, 5), c63);
			c = vsubq_u8(c71, vrshrq_n_u8(a, 1));
			c = vaddq_u8(b, c);
			c = vandq_u8(c, vcltq_u8(a, c52));
			a = vaddq_u8(vsubq_u8(a, c6), c);
			vst1q_u8(d, a);
			s += 12; n -= 12; d += 16;
		} while (n >= 12);
	}
#elif CRZY64_VEC && defined(__AVX2__)
	if (n >= 24) {
		__m256i c1 = _mm256_set1_epi8(1), c5 = _mm256_set1_epi8(5);
		__m256i c52 = _mm256_set1_epi8(52), c74 = _mm256_set1_epi8(74);
		__m256i c63 = _mm256_set1_epi8(63), c6 = _mm256_set1_epi8(6), a, b, c;
		__m256i ml = _mm256_set1_epi32(0x030f3f);
		__m256i idx = _mm256_setr_epi8(
				4, 5, 6, -1, 7, 8, 9, -1, 10, 11, 12, -1, 13, 14, 15, -1,
				0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1);
		__m256i mask = _mm256_setr_epi32(0, -1, -1, -1, -1, -1, -1, 0);
		const uint8_t *end = s + n - 24; (void)end;
		do {
			a = _mm256_maskload_epi32((const int32_t*)s - 1, mask);
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			a = _mm256_shuffle_epi8(a, idx);
			/* unpack */
			c = _mm256_andnot_si256(ml, a);	/* fourth bytes are clear */
			b = _mm256_xor_si256(c, _mm256_slli_epi32(c, 6));
			b = _mm256_xor_si256(b, _mm256_slli_epi32(c, 12));
			c = _mm256_and_si256(a, ml);
			a = _mm256_xor_si256(c, _mm256_srli_epi32(c, 6));
			a = _mm256_xor_si256(a, _mm256_srli_epi32(c, 12));
			a = _mm256_xor_si256(a, _mm256_slli_epi32(b, 6));
			/* core */
			a = _mm256_and_si256(_mm256_add_epi8(a, c5), c63);
			b = _mm256_and_si256(a, c1);
			c = _mm256_add_epi8(_mm256_slli_epi16(b, 5), c74);
			c = _mm256_sub_epi8(c, _mm256_avg_epu8(a, c6));
			c = _mm256_and_si256(c, _mm256_cmpgt_epi8(c52, a));
			a = _mm256_add_epi8(_mm256_sub_epi8(a, c6), c);
			_mm256_storeu_si256((__m256i*)d, a);
			s += 24; n -= 24; d += 32;
		} while (n >= 24);
	}
#elif CRZY64_VEC && defined(__SSE2__)
	if (n >= 12) {
		__m128i c1 = _mm_set1_epi8(1), c5 = _mm_set1_epi8(5);
		__m128i c52 = _mm_set1_epi8(52), c74 = _mm_set1_epi8(74);
		__m128i c63 = _mm_set1_epi8(63), c6 = _mm_set1_epi8(6), a, b, c;
#ifdef __SSSE3__
		__m128i idx = _mm_setr_epi8(0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1);
#else
		__m128i mh = _mm_set1_epi32(0xfcf0c0);
#endif
		__m128i ml = _mm_set1_epi32(0x030f3f);
		do {
#ifdef __SSSE3__
			a = _mm_loadl_epi64((const __m128i*)s);
#ifdef __SSE4_1__
			a = _mm_insert_epi32(a, *(uint32_t*)(s + 8), 2);
#else
			b = _mm_cvtsi32_si128(*(uint32_t*)(s + 8));
			a = _mm_unpacklo_epi64(a, b);
#endif
			a = _mm_shuffle_epi8(a, idx);
			/* unpack */
			c = _mm_andnot_si128(ml, a);	/* fourth bytes are clear */
#else
			b = _mm_cvtsi32_si128(*(const uint32_t*)s);
			c = _mm_cvtsi32_si128(*(const uint32_t*)(s + 3));
			a = _mm_unpacklo_epi32(b, c);
			b = _mm_cvtsi32_si128(*(const uint32_t*)(s + 6));
			c = _mm_cvtsi32_si128(*(const uint32_t*)(s + 8));
			b = _mm_unpacklo_epi32(b, _mm_srli_epi32(c, 8));
			a = _mm_unpacklo_epi64(a, b);
			/* unpack */
			c = _mm_and_si128(a, mh);
#endif
			b = _mm_xor_si128(c, _mm_slli_epi32(c, 6));
			b = _mm_xor_si128(b, _mm_slli_epi32(c, 12));
			c = _mm_and_si128(a, ml);
			a = _mm_xor_si128(c, _mm_srli_epi32(c, 6));
			a = _mm_xor_si128(a, _mm_srli_epi32(c, 12));
			a = _mm_xor_si128(a, _mm_slli_epi32(b, 6));
			/* core */
			a = _mm_and_si128(_mm_add_epi8(a, c5), c63);
			b = _mm_and_si128(a, c1);
			c = _mm_add_epi8(_mm_slli_epi16(b, 5), c74);
			c = _mm_sub_epi8(c, _mm_avg_epu8(a, c6));
			c = _mm_and_si128(c, _mm_cmpgt_epi8(c52, a));
			a = _mm_add_epi8(_mm_sub_epi8(a, c6), c);
			_mm_storeu_si128((__m128i*)d, a);
			s += 12; n -= 12; d += 16;
		} while (n >= 12);
	}
#endif

#if CRZY64_FAST64
	if (n >= 6) do {
		uint64_t a, b, c;
#if CRZY64_UNALIGNED && !defined(CRZY64_E2K_LCC)
		a = *(const uint32_t*)s | (uint64_t)*(const uint32_t*)(s + 2) << 24;
#else
		a = s[0] | s[1] << 8 | s[2] << 16;
		a |= (uint64_t)(s[3] | s[4] << 8 | s[5] << 16) << 32;
#endif
		a = crzy64_unpack64(a);
		CRZY64_ENC(CRZY64_REP8);
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
		CRZY64_ENC(CRZY64_REP4);
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
		CRZY64_ENC(CRZY64_REP4);
		d[0] = a;
		d[1] = a >> 8;
		if (n > 1) d[2] = a >> 16;
		d += n + 1;
	}

	return d - d0;
}

#define CRZY64_DEC(a, R) (((((a) >> 5) & R(3)) + \
	(a) + (((a) - R(9)) & (R(64) - (((a) >> 6) & R(1))))) & R(63))
#define CRZY64_DEC4(a) CRZY64_DEC(a, CRZY64_REP4)
#define CRZY64_DEC8(a) CRZY64_DEC(a, CRZY64_REP8)
#define CRZY64_PACK(a) ((a) ^ (a) >> 6)

CRZY64_ATTR
size_t crzy64_decode(uint8_t *CRZY64_RESTRICT d,
		const uint8_t *CRZY64_RESTRICT s, size_t n) {
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
		const uint8_t *end = s + n - 16; (void)end;
		do {
			a = vld1q_u8(s);
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			b = vandq_u8(vsubq_u8(a, c9), vcltq_u8(c63, a));
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
#if 0 // worse
		if (n) {
			int32_t x;
			uint8x8_t idx2 = vcreate_u8(0x0b0a090807060504);
			uint8x8_t idx3 = vcreate_u8(0x131211100f0e0d0c);
			b = vcombine_u8(idx2, idx3);
			a = vld1q_u8(s + n - 16);
			d += (n >> 2) * 3; n &= 3;
			b = vsubq_u8(b, vdupq_n_u8(n));
#ifdef __aarch64__
			a = vqtbl1q_u8(a, b);
#else
			c.c = a;
			a = vcombine_u8(vtbl2_u8(c.b, vget_low_u8(b)),
				vtbl2_u8(c.b, vget_high_u8(b)));
#endif
			b = vandq_u8(vsubq_u8(a, c9), vcltq_u8(c63, a));
			a = vaddq_u8(vaddq_u8(a, vshrq_n_u8(a, 5)), b);
			a = vandq_u8(a, c63);
			c.a = vreinterpretq_u32_u8(a);
			c.a = veorq_u32(c.a, vshrq_n_u32(c.a, 6));
#ifdef __aarch64__
			c.c = vqtbl1q_u8(c.c, idx);
			vst1q_lane_u64((uint64_t*)(d - 9), c.d, 0);
			x = vgetq_lane_u32(c.a, 2);
#else
			vst1_u8(d - 9, vtbl2_u8(c.b, idx0));
			x = vget_lane_u32(vreinterpret_u32_u8(vtbl2_u8(c.b, idx1)), 0);
#endif
			d[-1] = x;
			if (n > 1) {
				d += n - 1;
				*(uint16_t*)(d - 2) = x >> ((n << 3) - 16);
			}
		}
		return d - d0;
#endif
	}
#elif CRZY64_VEC && defined(__AVX2__)
	if (n >= 32) {
		__m256i c3 = _mm256_set1_epi8(3), c9 = _mm256_set1_epi8(9);
		__m256i c63 = _mm256_set1_epi8(63), a, b, c;
		__m256i idx = _mm256_setr_epi8(
				-1, -1, -1, -1, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14,
				0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1);
		__m256i mask = _mm256_cmpgt_epi32(idx, c3);
		const uint8_t *end = s + n - 32; (void)end;

#define CRZY64_DEC_AVX2() do { \
	b = _mm256_and_si256(_mm256_srli_epi16(a, 5), c3); \
	c = _mm256_and_si256(_mm256_sub_epi8(a, c9), _mm256_cmpgt_epi8(a, c63)); \
	a = _mm256_add_epi8(_mm256_add_epi8(a, b), c); \
	a = _mm256_and_si256(a, c63); \
	a = _mm256_xor_si256(a, _mm256_srli_epi32(a, 6)); \
	a = _mm256_shuffle_epi8(a, idx); \
} while (0)

#if CRZY64_UNROLL > 1
		while (n >= 64) {
			__m256i a1;
			a = _mm256_loadu_si256((const __m256i*)s); s += 32;
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			CRZY64_DEC_AVX2();
			a1 = _mm256_loadu_si256((const __m256i*)s); s += 32;
			_mm256_maskstore_epi32((int32_t*)d - 1, mask, a); d += 24;
			a = a1;
			CRZY64_DEC_AVX2();
			_mm256_maskstore_epi32((int32_t*)d - 1, mask, a); d += 24;
			n -= 64;
		}
		if (n >= 32) {
			a = _mm256_loadu_si256((const __m256i*)s);
			CRZY64_DEC_AVX2();
			_mm256_maskstore_epi32((int32_t*)d - 1, mask, a);
			s += 32; n -= 32; d += 24;
		}
#else
		do {
			a = _mm256_loadu_si256((const __m256i*)s);
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			CRZY64_DEC_AVX2();
			_mm256_maskstore_epi32((int32_t*)d - 1, mask, a);
			s += 32; n -= 32; d += 24;
		} while (n >= 32);
#endif
		if (n) {
			int32_t x; __m128i a1, b1, c1;
			b1 = _mm_setr_epi8(
					0x74, 0x75, 0x76, 0x77,  0x78,  0x79,  0x7a,  0x7b,
					0x7c, 0x7d, 0x7e, 0x7f, -0x80, -0x7f, -0x7e, -0x7d);
			s += n;
			d += (n >> 2) * 3; n &= 3;
			c1 = _mm_loadu_si128((const __m128i*)(s - n - 28));
			a1 = _mm_loadu_si128((const __m128i*)s - 1);
			b1 = _mm_sub_epi8(b1, _mm_set1_epi8(n));
			a1 = _mm_shuffle_epi8(a1, b1);
			a = _mm256_castsi128_si256(c1);
			a = _mm256_inserti128_si256(a, a1, 1);
			CRZY64_DEC_AVX2();
			c1 = _mm256_castsi256_si128(a);
			b1 = _mm256_castsi256_si128(mask);
			a1 = _mm256_extracti128_si256(a, 1);
			_mm_maskstore_epi32((int32_t*)(d - 9) - 4, b1, c1);
			_mm_storel_epi64((__m128i*)(d - 9), a1);
			x = _mm_extract_epi32(a1, 2);
			d[-1] = x;
			if (n > 1) {
				d += n - 1;
				*(uint16_t*)(d - 2) = x >> ((n << 3) - 16);
			}
		}
		return d - d0;
	}
#elif CRZY64_VEC && defined(__SSE2__)
	if (n >= 16) {
		__m128i c3 = _mm_set1_epi8(3), c9 = _mm_set1_epi8(9);
		__m128i c63 = _mm_set1_epi8(63), a, b, c;
#ifdef __SSSE3__
		__m128i idx = _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1);
#else
		/* xx0001x0011x0111 */
		__m128i mask = _mm_setr_epi32(0xffffff, 0xffffff, 0xff0000, 0);
#endif

#define CRZY64_DEC_SSE2() do { \
	b = _mm_and_si128(_mm_srli_epi16(a, 5), c3); \
	c = _mm_and_si128(_mm_sub_epi8(a, c9), _mm_cmpgt_epi8(a, c63)); \
	a = _mm_add_epi8(_mm_add_epi8(a, b), c); \
	a = _mm_and_si128(a, c63); \
	a = _mm_xor_si128(a, _mm_srli_epi32(a, 6)); \
} while (0)

#if CRZY64_UNROLL > 1 && defined(__SSSE3__)
		const uint8_t *end = s + n - 16; (void)end;
		while (n >= 32 + 6) {
			__m128i a1;
			a = _mm_loadu_si128((const __m128i*)s); s += 16;
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			CRZY64_DEC_SSE2();
			a = _mm_shuffle_epi8(a, idx);
			a1 = _mm_loadu_si128((const __m128i*)s); s += 16;
			_mm_storeu_si128((__m128i*)d, a); d += 12;
			a = a1;
			CRZY64_DEC_SSE2();
			a = _mm_shuffle_epi8(a, idx);
			_mm_storeu_si128((__m128i*)d, a); d += 12;
			n -= 32;
		}
		while (n >= 16) {
			a = _mm_loadu_si128((const __m128i*)s);
			CRZY64_DEC_SSE2();
			a = _mm_shuffle_epi8(a, idx);
			_mm_storel_epi64((__m128i*)d, a);
#ifdef __SSE4_1__
			*(uint32_t*)(d + 8) = _mm_extract_epi32(a, 2);
#else
			a = _mm_bsrli_si128(a, 8);
			*(uint32_t*)(d + 8) = _mm_cvtsi128_si32(a);
#endif
			s += 16; n -= 16; d += 12;
		}
#else
		do {
			a = _mm_loadu_si128((const __m128i*)s);
			CRZY64_DEC_SSE2();
#ifdef __SSSE3__
			a = _mm_shuffle_epi8(a, idx);
			_mm_storel_epi64((__m128i*)d, a);
#ifdef __SSE4_1__
			*(uint32_t*)(d + 8) = _mm_extract_epi32(a, 2);
#else
			a = _mm_bsrli_si128(a, 8);
			*(uint32_t*)(d + 8) = _mm_cvtsi128_si32(a);
#endif
#else
			b = _mm_andnot_si128(mask, _mm_bsrli_si128(a, 1));
			a = _mm_or_si128(_mm_and_si128(a, mask), b);
			*(uint32_t*)d = _mm_cvtsi128_si32(a);
			b = _mm_bsrli_si128(a, 5);
			c = _mm_bsrli_si128(a, 10);
			*(uint32_t*)(d + 4) = _mm_cvtsi128_si32(b);
			*(uint32_t*)(d + 8) = _mm_cvtsi128_si32(c);
#endif
			s += 16; n -= 16; d += 12;
		} while (n >= 16);
#endif
#ifdef __SSSE3__
		if (n) {
			int32_t x;
			b = _mm_setr_epi8(
					0x74, 0x75, 0x76, 0x77,  0x78,  0x79,  0x7a,  0x7b,
					0x7c, 0x7d, 0x7e, 0x7f, -0x80, -0x7f, -0x7e, -0x7d);
			a = _mm_loadu_si128((const __m128i*)(s + n) - 1);
			d += (n >> 2) * 3; n &= 3;
			b = _mm_sub_epi8(b, _mm_set1_epi8(n));
			a = _mm_shuffle_epi8(a, b);
			CRZY64_DEC_SSE2();
			a = _mm_shuffle_epi8(a, idx);
			_mm_storel_epi64((__m128i*)(d - 9), a);
#ifdef __SSE4_1__
			x = _mm_extract_epi32(a, 2);
#else
			b = _mm_bsrli_si128(a, 8);
			x = _mm_cvtsi128_si32(b);
#endif
			d[-1] = x;
			if (n > 1) {
				d += n - 1;
				*(uint16_t*)(d - 2) = x >> ((n << 3) - 16);
			}
		}
		return d - d0;
#endif
	}
#endif
#if CRZY64_FAST64
#if CRZY64_UNROLL > 1 && !CRZY64_VEC && CRZY64_UNALIGNED
	if (n >= 16) do {
		uint64_t a = *(const uint64_t*)s, b;
		b = *(const uint64_t*)(s + 8);
		a = CRZY64_DEC8(a); a = CRZY64_PACK(a);
		b = CRZY64_DEC8(b); b = CRZY64_PACK(b);
		*(uint32_t*)d = ((uint32_t)a & 0xffffff)
				| ((uint32_t)(a >> 8) & ~0xffffff);
		*(uint32_t*)(d + 4) = b << 16 | (a >> 40 & 0xffff);
		*(uint32_t*)(d + 8) = ((uint32_t)(b >> 24) & ~0xff)
				| ((uint32_t)b >> 16 & 0xff);
		s += 16; n -= 16; d += 12;
	} while (n >= 16);
#endif

	if (n >= 8) do {
#if CRZY64_UNALIGNED
		uint64_t a = *(const uint64_t*)s;
#else
		uint64_t a = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
		a |= (uint64_t)(s[4] | s[5] << 8 | s[6] << 16 | s[7] << 24) << 32;
#endif
		a = CRZY64_DEC8(a);
		a = CRZY64_PACK(a);
#if CRZY64_UNALIGNED
#ifdef CRZY64_E2K_LCC
		a = __builtin_e2k_insfd(a, 8 | 24 << 6, a);
		*(uint16_t*)d = a;
		*(uint16_t*)(d + 2) = a >> 16;
		*(uint16_t*)(d + 4) = a >> 32;
#else
		*(uint32_t*)d = ((uint32_t)a & 0xffffff)
				| ((uint32_t)(a >> 8) & ~0xffffff);
		*(uint16_t*)(d + 4) = a >> 40;
#endif
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
#if CRZY64_UNALIGNED
		uint64_t a = *(const uint32_t*)s;
		a |= (uint64_t)*(const uint32_t*)(s + n - 4) << ((n << 3) - 32);
#else
		uint64_t a = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
		a |= (uint64_t)(s[4] | s[5] << 8) << 32;
		if (n > 6) a |= (uint64_t)s[6] << 48;
#endif
		a = CRZY64_DEC8(a);
		a = CRZY64_PACK(a);
#if CRZY64_UNALIGNED
		*(uint32_t*)d = a;
#else
		d[0] = a;
		d[1] = a >> 8;
		d[2] = a >> 16;
#endif
		d[3] = a >> 32;
		if (n > 6) d[4] = a >> 40;
		d += n - 2;
	} else if (n > 1) {
		uint32_t a;
#if CRZY64_UNALIGNED
		a = *(const uint16_t*)s;
		uint32_t x = *(const uint16_t*)(s + n - 2);
		a |= x << ((n << 3) - 16);
#else
		a = s[0] | s[1] << 8;
		if (n > 2) a |= s[2] << 16;
		if (n > 3) a |= s[3] << 24;
#endif
		a = CRZY64_DEC4(a);
		a = CRZY64_PACK(a);
		d[0] = a;
#if CRZY64_UNALIGNED
		d += n - 1;
		if (n > 2)
			*(uint16_t*)(d - 2) = a >> ((n << 3) - 24);
#else
		if (n > 2) d[1] = a >> 8;
		if (n > 3) d[2] = a >> 16;
		d += n - 1;
#endif
	}
#else
	if (n >= 4) do {
#if CRZY64_UNALIGNED
		uint32_t a = *(const uint32_t*)s;
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

#endif // CRZY64_H
