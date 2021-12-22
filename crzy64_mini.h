/*
 * Copyright (c) 2021, Ilya Kurdyukov
 * All rights reserved.
 *
 * crzy64: An easy to decode base64 modification. (mini version)
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

/* 24 -> 6x4 */
static inline uint32_t crzy64_unpack(uint32_t a) {
	uint32_t b;
	b = a & 0xfcf0c0;
	a = a & 0x030f3f;
	b = b ^ b << 6 ^ b << 12;
	a = a ^ a >> 6 ^ a >> 12;
	return (a ^ b << 6) & 0x3f3f3f3f;
}

#define CRZY64_ENC(R) do { \
	a = (a + R * 5) & R * 63; \
	b = a & R * 1; \
	c = R * 127 + (((a + R * 12) >> 6) & R * 1); \
	a += (((b << 5) + R * 71 - ((a + b) >> 1)) & c) - R * 6; \
} while (0)
#define CRZY64_ENC4() CRZY64_ENC(0x01010101)

size_t crzy64_encode(uint8_t *d, const uint8_t *s, size_t n) {
	uint8_t *d0 = d;
	uint32_t a, b, c;

	while (n >= 3) {
		a = s[0] | s[1] << 8 | s[2] << 16;
		a = crzy64_unpack(a);
		CRZY64_ENC4();
		d[0] = a;
		d[1] = a >> 8;
		d[2] = a >> 16;
		d[3] = a >> 24;
		s += 3; n -= 3; d += 4;
	}

	if (n) {
		a = s[0];
		if (n > 1) a |= s[1] << 8;
		a = crzy64_unpack(a);
		CRZY64_ENC4();
		d[0] = a;
		d[1] = a >> 8;
		if (n > 1) d[2] = a >> 16;
		d += n + 1;
	}

	return d - d0;
}

#define CRZY64_DEC(a, R) (((((a) >> 5) & R * 3) + \
	(a) + (((a) - R * 9) & (R * 64 - (((a) >> 6) & R * 1)))) & R * 63)
#define CRZY64_DEC4(a) CRZY64_DEC(a, 0x01010101)
#define CRZY64_PACK(a) ((a) ^ (a) >> 6)

size_t crzy64_decode(uint8_t *d, const uint8_t *s, size_t n) {
	uint8_t *d0 = d;

	while (n >= 4) {
		uint32_t a = s[0] | s[1] << 8 | s[2] << 16 | s[3] << 24;
		a = CRZY64_DEC4(a);
		a = CRZY64_PACK(a);
		d[0] = a;
		d[1] = a >> 8;
		d[2] = a >> 16;
		s += 4; n -= 4; d += 3;
	}

	if (n > 1) {
		uint32_t a = s[0] | s[1] << 8;
		if (n > 2) a |= s[2] << 16;
		a = CRZY64_DEC4(a);
		a = CRZY64_PACK(a);
		d[0] = a;
		if (n > 2) d[1] = a >> 8;
		d += n - 1;
	}

	return d - d0;
}

#endif // CRZY64_H
