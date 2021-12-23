#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "crzy64.h"

#ifndef CRZY64_FAST64
#define CRZY64_FAST64 0
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static int64_t get_time_usec() {
	LARGE_INTEGER freq, perf;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&perf);
	return perf.QuadPart * 1000000.0 / freq.QuadPart;
}
#else
#include <time.h>
#include <sys/time.h>
static int64_t get_time_usec() {
	struct timeval time;
	gettimeofday(&time, NULL);
	return time.tv_sec * (int64_t)1000000 + time.tv_usec;
}
#endif

int main(int argc, char **argv) {
#if CRZY64_VEC && CRZY64_NEON
	const char *vec_type = "neon";
#elif CRZY64_VEC && defined(__AVX2__)
	const char *vec_type = "avx2";
#elif CRZY64_VEC && defined(__SSSE3__)
	const char *vec_type = "ssse3";
#elif CRZY64_VEC && defined(__SSE2__)
	const char *vec_type = "sse2";
#else
	const char *vec_type = "none";
#endif
	size_t i, n = 100, n1, n2;
	uint8_t *buf, *out;
	unsigned nrep = 5;
	int64_t t0, t1;

	if (argc > 1) n = atoi(argv[1]);
	if (argc > 2) nrep = atoi(argv[2]);
	if ((nrep - 1) >= 100) return 1;
	if (!n || n > 877) return 1; /* >= 2GB */

	printf("vector: %s\nfast64: %s\nsize: %u MB\n\n",
			vec_type, CRZY64_FAST64 ? "yes" : "no", (int)n);

	n1 = n << 20;
	n2 = (n1 * 4 + 2) / 3;

	if (!(buf = malloc(n1 + n2 + 1))) return 1;
	out = buf + n1;
	for (i = 0; i < n1 + n2; i++) buf[i] = i ^ 0x55;

#define BENCH(name, code) \
	for (t1 = i = 0; i < nrep; i++) { \
		t0 = get_time_usec(); \
		code; \
		t0 = get_time_usec() - t0; \
		if (!i || t0 < t1) t1 = t0; \
	} \
	printf(name ": %.3fms (%.2f MB/s)\n", t1 * 0.001, n * 1e6 / t1);

	BENCH("memcpy", memcpy(out, buf, n1))
	BENCH("encode", crzy64_encode(out, buf, n1))
	BENCH("decode", crzy64_decode(buf, out, n2))
#if CRZY64_UNALIGNED
	BENCH("encode_unaligned", crzy64_encode(out + 1, buf + 1, n1))
	BENCH("decode_unaligned", crzy64_decode(buf + 1, out + 1, n2))
#endif
#undef BENCH
}
