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
#define CRZY64_UNROLL 4
#else
#define CRZY64_UNROLL 0
#endif
#endif

#ifndef CRZY64_BRANCHLESS
#if !defined(CRZY64_E2K_LCC)
#define CRZY64_BRANCHLESS 1
#else
#define CRZY64_BRANCHLESS 0
#endif
#endif

/* 24 -> 6x4 */
static inline uint32_t crzy64_unpack(uint32_t a) {
	uint32_t b = a << 6, m;
	b &= m = 0xfcf0c0 << 6;
	a &=     0x030f3f;
	b = b ^ b << 6 ^ b << 12;
	a = a ^ a >> 6 ^ a >> 12;
	return a ^ (b & m);
}

#if CRZY64_FAST64
/* 24x2 -> 6x8 */
static inline uint64_t crzy64_unpack64(uint64_t a) {
	// return crzy64_unpack(a) | (uint64_t)crzy64_unpack(a >> 32) << 32;
#if defined(CRZY64_E2K_LCC) && __iset__ >= 6
	uint64_t b, m = 0x3ffff0003ffff000;
	b = __builtin_e2k_clmull(a & 0xfcf0c000fcf0c0, 0x1041 << 6) & m;
	a = __builtin_e2k_clmull(a & 0x030f3f00030f3f, 0x1041) & m;
	return a >> 12 ^ b;
#else
	uint64_t b = a << 6, h, l;
	b &= h = 0xfcf0c000fcf0c0 << 6;
	a &= l = 0x030f3f00030f3f;
	b = (b ^ b << 6 ^ b << 12) & h;
	a = (a ^ a >> 6 ^ a >> 12) & l;
	return a ^ b;
#endif
}
#endif

#define CRZY64_DEC1(a) ((a) - 59 \
	+ ((7 + ((a) >> 6)) & 7) + ((5 + ((a) >> 5)) & 6))

#define CRZY64_REP4(x) (x * 0x01010101)
#define CRZY64_REP8(x) (x * 0x0101010101010101)
#define CRZY64_ENC(R) do { \
	b = R(64) - ((a + R(52)) >> 6 & R(1)); \
	c = R(64) - ((a + R(26)) >> 6 & R(1)); \
	a += R(46) + (b & R(7)) + (c & R(6)); \
} while (0)
#define CRZY64_ENC4() CRZY64_ENC(CRZY64_REP4)
#ifdef CRZY64_E2K_LCC
#define CRZY64_ENC_E2K(R) do { \
	b = __builtin_e2k_pcmpgtb(a, R(11)); \
	c = __builtin_e2k_pcmpgtb(a, R(37)); \
	a += R(46) + (b & R(7)) + (c & R(6)); \
} while (0)
#define CRZY64_ENC8() CRZY64_ENC_E2K(CRZY64_REP8)
#else
#define CRZY64_ENC8() CRZY64_ENC(CRZY64_REP8)
#endif

CRZY64_ATTR
size_t crzy64_encode(uint8_t *CRZY64_RESTRICT d,
		const uint8_t *CRZY64_RESTRICT s, size_t n) {
	uint8_t *d0 = d; uint32_t a, b, c;

#if CRZY64_VEC && CRZY64_NEON
	if (n >= 12) {
		uint8x16_t c11 = vdupq_n_u8(11), c37 = vdupq_n_u8(37);
		uint8x16_t c46 = vdupq_n_u8(46), c63 = vdupq_n_u8(63);
		uint8x16_t c52 = vdupq_n_u8(52), a, b, c;
		uint8x8_t idx0 = vcreate_u8(0xff050403ff020100);
#ifdef __aarch64__
		uint8x8_t idx1 = vcreate_u8(0xff0b0a09ff080706);
		uint8x16_t idx = vcombine_u8(idx0, idx1);
#else
		uint8x8_t idx1 = vcreate_u8(0xff070605ff040302);
#endif
		uint32x4_t ml = vdupq_n_u32(0x030f3f), x, y, z;
		const uint8_t *end = s + n - 12; (void)end;

#ifdef __aarch64__
#define CRZY64_ENC_NEON_LD(a) do { \
	a = vcombine_u8(vld1_u8(s), \
			vcreate_u8(*(const uint32_t*)(s + 8))); \
	a = vqtbl1q_u8(a, idx); \
} while (0)
#else
#define CRZY64_ENC_NEON_LD(a) do { \
	a = vcombine_u8(vtbl1_u8(vld1_u8(s), idx0), \
			vtbl1_u8(vld1_u8(s + 4), idx1)); \
} while (0)
#endif

#define CRZY64_ENC_NEON() do { \
	/* unpack */ \
	x = vreinterpretq_u32_u8(a); \
	z = vbicq_u32(x, ml); \
	y = veorq_u32(z, vshlq_n_u32(z, 6)); \
	y = veorq_u32(y, vshlq_n_u32(z, 12)); \
	z = vandq_u32(x, ml); \
	x = veorq_u32(z, vshrq_n_u32(z, 6)); \
	x = veorq_u32(x, vshrq_n_u32(z, 12)); \
	x = veorq_u32(x, vshlq_n_u32(y, 6)); \
	a = vreinterpretq_u8_u32(x); \
	/* core */ \
	a = vandq_u8(a, c63); \
	b = vsraq_n_u8(a, vcltq_u8(c11, a), 5); \
	c = vbslq_u8(vcltq_u8(c37, a), c52, c46); \
	a = vaddq_u8(b, c); \
} while (0)

#if CRZY64_UNROLL > 3
		uint8x16_t c3 = vdupq_n_u8(3), c15 = vdupq_n_u8(15);
		while (n >= 48) {
			uint8x16x3_t q0 = vld3q_u8(s);
			uint8x16x4_t q1;

			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			a = vandq_u8(vshlq_n_u8(q0.val[2], 2), c15);
			b = vbicq_u8(vshrq_n_u8(q0.val[0], 2), c15);
			c = veorq_u8(q0.val[1], veorq_u8(a, b));
			a = vshlq_n_u8(vandq_u8(c, c15), 2);
			b = vshrq_n_u8(vbicq_u8(c, c15), 2);
			a = veorq_u8(q0.val[0], a);
			b = veorq_u8(q0.val[2], b);

			q1.val[0] = vandq_u8(a, c63);
			q1.val[1] = vbslq_u8(c15, c, vshrq_n_u8(a, 2));
			q1.val[2] = vbslq_u8(c3, b, vshrq_n_u8(c, 2));
			q1.val[3] = vshrq_n_u8(b, 2);

#define CRZY64_ENC_T(a) do { \
	a = vandq_u8(a, c63); \
	b = vsraq_n_u8(a, vcltq_u8(c11, a), 5); \
	c = vbslq_u8(vcltq_u8(c37, a), c52, c46); \
	a = vaddq_u8(b, c); \
} while (0)
			CRZY64_ENC_T(q1.val[0]);
			CRZY64_ENC_T(q1.val[1]);
			CRZY64_ENC_T(q1.val[2]);
			CRZY64_ENC_T(q1.val[3]);
#undef CRZY64_ENC_T
			vst4q_u8(d, q1);
			s += 48; n -= 48; d += 64;
		}
#endif

#if CRZY64_UNROLL > 1
		while (n >= 24) {
			uint8x16_t a1;
			CRZY64_ENC_NEON_LD(a); s += 12;
			CRZY64_ENC_NEON_LD(a1); s += 12;
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			CRZY64_ENC_NEON();
			vst1q_u8(d, a); d += 16;
			a = a1;
			CRZY64_ENC_NEON();
			vst1q_u8(d, a); d += 16;
			n -= 24;
		}
		if (n >= 12) {
			CRZY64_ENC_NEON_LD(a);
			CRZY64_ENC_NEON();
			vst1q_u8(d, a);
			s += 12; n -= 12; d += 16;
		}
#else
		do {
			CRZY64_ENC_NEON_LD(a);
			CRZY64_ENC_NEON();
			vst1q_u8(d, a);
			s += 12; n -= 12; d += 16;
		} while (n >= 12);
#endif
	}
#elif CRZY64_VEC && defined(__AVX2__)
	if (n >= 24) {
		__m256i c11 = _mm256_set1_epi8(11), c37 = _mm256_set1_epi8(37);
		__m256i c46 = _mm256_set1_epi8(46), c63 = _mm256_set1_epi8(63);
		__m256i c6 = _mm256_set1_epi8(6), c7 = _mm256_set1_epi8(7), a, b, c;
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
			a = _mm256_and_si256(a, c63);
			b = _mm256_and_si256(_mm256_cmpgt_epi8(a, c11), c7);
			c = _mm256_and_si256(_mm256_cmpgt_epi8(a, c37), c6);
			a = _mm256_add_epi8(a, c46);
			a = _mm256_add_epi8(_mm256_add_epi8(a, b), c);
			_mm256_storeu_si256((__m256i*)d, a);
			s += 24; n -= 24; d += 32;
		} while (n >= 24);
	}
#elif CRZY64_VEC && defined(__SSE2__)
	if (n >= 12) {
		__m128i c11 = _mm_set1_epi8(11), c37 = _mm_set1_epi8(37);
		__m128i c46 = _mm_set1_epi8(46), c63 = _mm_set1_epi8(63);
		__m128i c6 = _mm_set1_epi8(6), c7 = _mm_set1_epi8(7), a, b, c;
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
			a = _mm_and_si128(a, c63);
			b = _mm_and_si128(_mm_cmpgt_epi8(a, c11), c7);
			c = _mm_and_si128(_mm_cmpgt_epi8(a, c37), c6);
			a = _mm_add_epi8(a, c46);
			a = _mm_add_epi8(_mm_add_epi8(a, b), c);
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
		CRZY64_ENC8();
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

	if (n >= 3) {
#else
	while (n >= 3) {
#endif
		a = s[0] | s[1] << 8 | s[2] << 16;
		a = crzy64_unpack(a);
		CRZY64_ENC4();
#if CRZY64_UNALIGNED
		*(uint32_t*)d = a;
#else
		d[0] = a;
		d[1] = a >> 8;
		d[2] = a >> 16;
		d[3] = a >> 24;
#endif
		s += 3; n -= 3; d += 4;
	}

	if (n) {
		a = s[0];
#if CRZY64_BRANCHLESS
		a |= s[n - 1] << ((n << 3) - 8);
#else
		if (n > 1) a |= s[1] << 8;
#endif
		a = crzy64_unpack(a);
		CRZY64_ENC4();
#if CRZY64_UNALIGNED
		*(uint16_t*)d = a;
#else
		d[0] = a;
		d[1] = a >> 8;
#endif
#if CRZY64_BRANCHLESS
		d += n + 1;
		d[-1] = a >> (n << 3);
#else
		if (n > 1) d[2] = a >> 16;
		d += n + 1;
#endif
	}

	return d - d0;
}

#define CRZY64_DEC(a, b, R) (b = (a) & R(96), (a) - R(59) \
	+ ((R(7) + ((b) >> 6)) & R(7)) \
	+ ((R(5) + ((b) >> 5)) & R(6)))
#define CRZY64_DEC4(a, b) CRZY64_DEC(a, b, CRZY64_REP4)
#ifdef CRZY64_E2K_LCC
#define CRZY64_DEC_E2K(a, b, R) (b = (a) >> 5 & R(3), \
	(a) - __builtin_e2k_pshufb(0, 0x3b352e00, (b)))
#define CRZY64_DEC8(a, b) CRZY64_DEC_E2K(a, b, CRZY64_REP8)
#else
#define CRZY64_DEC8(a, b) CRZY64_DEC(a, b, CRZY64_REP8)
#endif
#define CRZY64_PACK(a) ((a) ^ (a) >> 6)

CRZY64_ATTR
size_t crzy64_decode(uint8_t *CRZY64_RESTRICT d,
		const uint8_t *CRZY64_RESTRICT s, size_t n) {
	uint8_t *d0 = d;
#if CRZY64_VEC && CRZY64_NEON
	if (n >= 16) {
		uint8x16_t a, b;
		union {
			uint32x4_t a; uint8x8x2_t b;
			uint8x16_t c; uint64x2_t d;
		} c;
		uint8x8_t idx0 = vcreate_u8(0x0908060504020100);
		uint8x8_t idx1 = vcreate_u8(0x0e0d0c0a);
		uint8x8_t tab0 = vcreate_u8(0xc5cbd200);
#ifdef __aarch64__
		uint8x16_t idx = vcombine_u8(idx0, idx1);
		uint8x16_t tab = vcombine_u8(tab0, tab0);
#endif
		const uint8_t *end = s + n - 16; (void)end;

#ifdef __aarch64__
#define CRZY64_DEC_NEON() do { \
	b = vshrq_n_u8(a, 5); \
	c.c = vaddq_u8(a, vqtbl1q_u8(tab, b)); \
	c.a = veorq_u32(c.a, vshrq_n_u32(c.a, 6)); \
} while (0)
#define CRZY64_DEC_NEON_ST() do { \
	c.c = vqtbl1q_u8(c.c, idx); \
	vst1q_lane_u64((uint64_t*)d, c.d, 0); \
	vst1q_lane_u32((uint32_t*)d + 2, c.a, 2); \
} while (0)
#else
#define CRZY64_DEC_NEON() do { \
	c.c = vshrq_n_u8(a, 5); \
	b = vcombine_u8(vtbl1_u8(tab0, c.b.val[0]), \
			vtbl1_u8(tab0, c.b.val[1])); \
	c.c = vaddq_u8(a, b); \
	c.a = veorq_u32(c.a, vshrq_n_u32(c.a, 6)); \
} while (0)
#define CRZY64_DEC_NEON_ST() do { \
	vst1_u8(d, vtbl2_u8(c.b, idx0)); \
	vst1_lane_u32((uint32_t*)d + 2, \
			vreinterpret_u32_u8(vtbl2_u8(c.b, idx1)), 0); \
} while (0)
#endif

#if CRZY64_UNROLL > 1
		while (n >= 32) {
			uint8x16_t a1;
			a = vld1q_u8(s); s += 16;
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			CRZY64_DEC_NEON();
			a1 = vld1q_u8(s); s += 16;
			CRZY64_DEC_NEON_ST(); d += 12;
			a = a1;
			CRZY64_DEC_NEON();
			CRZY64_DEC_NEON_ST(); d += 12;
			n -= 32;
		}
		if (n >= 16) {
			a = vld1q_u8(s);
			CRZY64_DEC_NEON();
			CRZY64_DEC_NEON_ST();
			s += 16; n -= 16; d += 12;
		}
#else
		do {
			a = vld1q_u8(s);
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			CRZY64_DEC_NEON();
			CRZY64_DEC_NEON_ST();
			s += 16; n -= 16; d += 12;
		} while (n >= 16);
#endif
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
			CRZY64_DEC_NEON();
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
		__m256i c3 = _mm256_set1_epi8(3), a, b;
		__m256i tab = _mm256_set1_epi32(0xc5cbd200);
		__m256i idx = _mm256_setr_epi8(
				-1, -1, -1, -1, 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14,
				0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1);
		__m256i mask = _mm256_cmpgt_epi32(idx, c3);
		const uint8_t *end = s + n - 32; (void)end;

#define CRZY64_DEC_AVX2(a) ( \
	b = _mm256_and_si256(_mm256_srli_epi16(a, 5), c3), \
	a = _mm256_add_epi8(a, _mm256_shuffle_epi8(tab, b)), \
	a = _mm256_xor_si256(a, _mm256_srli_epi32(a, 6)), \
	_mm256_shuffle_epi8(a, idx))

#if CRZY64_UNROLL > 1 && CRZY64_UNROLL <= 4
		while (n >= 32 * CRZY64_UNROLL) {
			__m256i a1;
			a = _mm256_loadu_si256((const __m256i*)s); s += 32;
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			a = CRZY64_DEC_AVX2(a);
#if CRZY64_UNROLL > 2
			a1 = _mm256_loadu_si256((const __m256i*)s); s += 32;
			_mm256_maskstore_epi32((int32_t*)d - 1, mask, a); d += 24;
			a = CRZY64_DEC_AVX2(a1);
#endif
#if CRZY64_UNROLL > 3
			a1 = _mm256_loadu_si256((const __m256i*)s); s += 32;
			_mm256_maskstore_epi32((int32_t*)d - 1, mask, a); d += 24;
			a = CRZY64_DEC_AVX2(a1);
#endif
			a1 = _mm256_loadu_si256((const __m256i*)s); s += 32;
			_mm256_maskstore_epi32((int32_t*)d - 1, mask, a); d += 24;
			a = CRZY64_DEC_AVX2(a1);
			_mm256_maskstore_epi32((int32_t*)d - 1, mask, a); d += 24;
			n -= 32 * CRZY64_UNROLL;
		}
#if CRZY64_UNROLL == 2
		if (n >= 32) {
#else
		while (n >= 32) {
#endif
			a = _mm256_loadu_si256((const __m256i*)s);
			a = CRZY64_DEC_AVX2(a);
			_mm256_maskstore_epi32((int32_t*)d - 1, mask, a);
			s += 32; n -= 32; d += 24;
		}
#else
		do {
			a = _mm256_loadu_si256((const __m256i*)s);
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			a = CRZY64_DEC_AVX2(a);
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
			a = CRZY64_DEC_AVX2(a);
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
#ifdef __SSSE3__
		__m128i c3 = _mm_set1_epi8(3), a, b;
		__m128i tab = _mm_set1_epi32(0xc5cbd200);
		__m128i idx = _mm_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1);

#define CRZY64_DEC_SSE2(a) ( \
	b = _mm_and_si128(_mm_srli_epi16(a, 5), c3), \
	a = _mm_add_epi8(a, _mm_shuffle_epi8(tab, b)), \
	_mm_xor_si128(a, _mm_srli_epi32(a, 6)))

#ifdef __SSE4_1__
#define CRZY64_DEC_SSE2_ST(a) do { \
	a = _mm_shuffle_epi8(a, idx); \
	_mm_storel_epi64((__m128i*)d, a); \
	*(uint32_t*)(d + 8) = _mm_extract_epi32(a, 2); \
} while (0)
#else
#define CRZY64_DEC_SSE2_ST(a) do { \
	a = _mm_shuffle_epi8(a, idx); \
	_mm_storel_epi64((__m128i*)d, a); \
	a = _mm_bsrli_si128(a, 8); \
	*(uint32_t*)(d + 8) = _mm_cvtsi128_si32(a); \
} while (0)
#endif
#define CRZY64_DEC_UNROLL_EXTRA 6
#define CRZY64_DEC_SSE2_ST1(a) do { \
	a = _mm_shuffle_epi8(a, idx); \
	_mm_storeu_si128((__m128i*)d, a); \
} while (0)
#else
		__m128i c59 = _mm_set1_epi8(59), a, b;
		__m128i c6 = _mm_set1_epi8(6), c7 = _mm_set1_epi8(7);
		__m128i c64 = _mm_set1_epi8(64), c96 = _mm_set1_epi8(96);
		/* xx0001x0011x0111 */
		__m128i mask = _mm_setr_epi32(0xffffff, 0xffffff, 0xff0000, 0);

#define CRZY64_DEC_SSE2(a) ( \
	b = _mm_and_si128(_mm_cmpgt_epi8(c64, a), c7), \
	a = _mm_add_epi8(_mm_add_epi8(b, _mm_sub_epi8(a, c59)), \
	_mm_and_si128(_mm_cmpgt_epi8(c96, a), c6)), \
	_mm_xor_si128(a, _mm_srli_epi32(a, 6)))

#define CRZY64_DEC_SSE2_ST(a) do { \
	b = _mm_andnot_si128(mask, _mm_bsrli_si128(a, 1)); \
	a = _mm_or_si128(_mm_and_si128(a, mask), b); \
	*(uint32_t*)d = _mm_cvtsi128_si32(a); \
	b = _mm_bsrli_si128(a, 5); \
	a = _mm_bsrli_si128(a, 10); \
	*(uint32_t*)(d + 4) = _mm_cvtsi128_si32(b); \
	*(uint32_t*)(d + 8) = _mm_cvtsi128_si32(a); \
} while (0)
#define CRZY64_DEC_UNROLL_EXTRA 0
#define CRZY64_DEC_SSE2_ST1 CRZY64_DEC_SSE2_ST
#endif

#if CRZY64_UNROLL > 1
		const uint8_t *end = s + n - 16; (void)end;
		while (n >= 32 + CRZY64_DEC_UNROLL_EXTRA) {
			__m128i a1;
			a = _mm_loadu_si128((const __m128i*)s); s += 16;
			CRZY64_PREFETCH(s + 1024 < end ? s + 1024 : end);
			a = CRZY64_DEC_SSE2(a);
			a1 = _mm_loadu_si128((const __m128i*)s); s += 16;
			CRZY64_DEC_SSE2_ST1(a); d += 12;
			a = CRZY64_DEC_SSE2(a1);
			CRZY64_DEC_SSE2_ST1(a); d += 12;
			n -= 32;
		}
#ifdef CRZY64_DEC_UNROLL_EXTRA
		while (n >= 16) {
#else
		if (n >= 16) {
#endif
			a = _mm_loadu_si128((const __m128i*)s);
			a = CRZY64_DEC_SSE2(a);
			CRZY64_DEC_SSE2_ST(a);
			s += 16; n -= 16; d += 12;
		}
#else
		do {
			a = _mm_loadu_si128((const __m128i*)s);
			a = CRZY64_DEC_SSE2(a);
			CRZY64_DEC_SSE2_ST(a);
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
			a = CRZY64_DEC_SSE2(a);
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
		uint64_t a = *(const uint64_t*)s, b, x;
		b = *(const uint64_t*)(s + 8);
		a = CRZY64_DEC8(a, x); a = CRZY64_PACK(a);
		b = CRZY64_DEC8(b, x); b = CRZY64_PACK(b);
		*(uint32_t*)d = ((uint32_t)a & 0xffffff)
				| ((uint32_t)(a >> 8) & ~0xffffff);
		*(uint32_t*)(d + 4) = b << 16 | (a >> 40 & 0xffff);
		*(uint32_t*)(d + 8) = ((uint32_t)(b >> 24) & ~0xff)
				| ((uint32_t)b >> 16 & 0xff);
		s += 16; n -= 16; d += 12;
	} while (n >= 16);
#endif

	if (n >= 8) do {
		uint64_t a, b;
#if CRZY64_UNALIGNED
		a = *(const uint64_t*)s;
#else
		a = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
		a |= (uint64_t)(s[4] | s[5] << 8 | s[6] << 16 | s[7] << 24) << 32;
#endif
		a = CRZY64_DEC8(a, b);
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
		uint64_t a, b;
#if CRZY64_UNALIGNED
		a = *(const uint32_t*)s;
		a |= (uint64_t)*(const uint32_t*)(s + n - 4) << ((n << 3) - 32);
#else
		a = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
		a |= (uint64_t)(s[4] | s[5] << 8) << 32;
#if CRZY64_BRANCHLESS
		a |= (uint64_t)s[n - 1] << ((n << 3) - 8);
#else
		if (n > 6) a |= (uint64_t)s[6] << 48;
#endif
#endif
		a = CRZY64_DEC8(a, b);
		a = CRZY64_PACK(a);
#if CRZY64_UNALIGNED
		*(uint32_t*)d = a;
#else
		d[0] = a;
		d[1] = a >> 8;
		d[2] = a >> 16;
#endif
		d[3] = a >> 32;
#if CRZY64_BRANCHLESS
		d[n - 3] = a >> ((n << 3) - 16);
#else
		if (n > 6) d[4] = a >> 40;
#endif
		d += n - 2;
	} else if (n > 1) {
		uint32_t a, b;
#if CRZY64_UNALIGNED
		a = *(const uint16_t*)s;
		a |= *(const uint16_t*)(s + n - 2) << ((n << 3) - 16);
#else
		a = s[0] | s[1] << 8;
		if (n > 2) a |= s[2] << 16;
		if (n > 3) a |= s[3] << 24;
#endif
		a = CRZY64_DEC4(a, b);
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
		uint32_t a, b;
#if CRZY64_UNALIGNED
		a = *(const uint32_t*)s;
#else
		a = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
#endif
		a = CRZY64_DEC4(a, b);
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
		uint32_t a, b;
#if CRZY64_UNALIGNED
		a = *(const uint16_t*)s;
#else
		a = s[0] | s[1] << 8;
#endif
#if CRZY64_BRANCHLESS
		a |= s[n - 1] << ((n << 3) - 8);
#else
		if (n > 2) a |= s[2] << 16;
#endif
		a = CRZY64_DEC4(a, b);
		a = CRZY64_PACK(a);
		d[0] = a;
#if CRZY64_BRANCHLESS
		d[n - 2] = a >> ((n << 3) - 16);
#else
		if (n > 2) d[1] = a >> 8;
#endif
		d += n - 1;
	}
#endif

	return d - d0;
}

#endif // CRZY64_H
