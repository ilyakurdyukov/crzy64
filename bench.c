#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "crzy64.h"

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
#elif CRZY64_VEC && defined(__SSSE3__)
	const char *vec_type = "ssse3";
#elif CRZY64_VEC && defined(__SSE2__)
	const char *vec_type = "sse2";
#else
	const char *vec_type = "none";
#endif
	size_t i, n = 100, n2;
	uint8_t *buf, *out;

	if (argc > 1) n = atoi(argv[1]);
	if (!n || n > 877) return 1; /* >= 2GB */

	printf("vector: %s\nfast64: %s\nsize: %u MB\n\n",
			vec_type, CRZY64_FAST64 ? "yes" : "no", (int)n);

	n <<= 20;
	n2 = (n + 2) / 3 << 2;

	if (!(buf = malloc(n + n2))) return 1;
	out = buf + n;
	for (i = 0; i < n + n2; i++) buf[i] = i ^ 0x55;

#define BENCH(name, code) \
	for (i = 0; i < 5; i++) { \
		int64_t t = get_time_usec(); \
		code; \
		t = get_time_usec() - t; \
		printf(name ": %.3fms\n", t * 0.001); \
	} \
	putchar('\n');

	BENCH("memcpy", memcpy(out, buf, n))
	BENCH("encode", crzy64_encode(out, buf, n))
	BENCH("decode", crzy64_decode(buf, out, n2))
#undef BENCH
}
