#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#ifdef TB32_BENCH
#include "turbob64.h"
#ifdef __AVX2__
#define crzy64_encode(d, s, n) tb64avx2enc(s, n, d)
#define crzy64_decode(d, s, n) tb64avx2dec(s, n, d)
#define TB32_NAME "tb64avx2"
#else
#define crzy64_encode(d, s, n) tb64sseenc(s, n, d)
#define crzy64_decode(d, s, n) tb64ssedec(s, n, d)
#define TB32_NAME "tb64sse"
#endif
#else
#include "crzy64.h"
#endif

#ifdef RDTSC_FREQ
/* useful if you set a static CPU frequency  */
#include <x86intrin.h>
#define TIMER_FREQ RDTSC_FREQ
static int64_t get_time_usec() {
	unsigned aux;
	return __rdtscp(&aux);
}
#else
#define TIMER_FREQ 1e6
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
#include <sys/time.h>
static int64_t get_time_usec() {
	struct timeval time;
	gettimeofday(&time, NULL);
	return time.tv_sec * (int64_t)1000000 + time.tv_usec;
}
#endif
#endif

static uint32_t randseed;

/* not perfect but should be enough */
static uint32_t bench_rand(uint32_t n) {
	uint32_t tmp;
	randseed = randseed * 0xdeece66d + 11;
	tmp = randseed & ~0u << 16;
	randseed = randseed * 0xdeece66d + 11;
	tmp |= randseed >> 16;
	return tmp * (uint64_t)n >> 32;
}

#include "bench_svg.h"
#include <math.h>

static void write_svg(const char *base, const char *name,
		float (*results)[21], float max) {
	svg_writer_t svg = {
		20, 10,
		100, 50, 50, /* x */
		100, 50, 70, /* y */
		0, 0, NULL
	};
	int i, j;
	float mul = 1.0f / 1024;
	uint64_t x;
	const char *unit = "KB/s";

	int n = strlen(base);
	char *buf = (char*)malloc(n + strlen(name) + 1);
	if (!buf) return;
	strcpy(buf, base);
	strcpy(buf + n, name);

	/* skip memcpy */
	if (max == 0)
	for (j = 1; j < 3; j++)
	for (i = 0; i <= 20; i++)
		if (max < results[j][i]) max = results[j][i];

	x = max * (1024 * 1.5f / svg.ny);
	x |= 1;
	for (i = -1; x; x >>= 1) i++;

	i = 1 << i;
	mul = 1024.0f / i;
	j = svg.ym * svg.ny;
	/* svg.ny = ceilf(max * mul); */
	svg.ny = 65536 - (int)(65536 - max * mul);
	svg.ym = j / svg.ny;
	if (i >= 1024) i >>= 10, unit = "MB/s";
	if (i >= 1024) i >>= 10, unit = "GB/s";
	if (i >= 1024) i >>= 10, unit = "TB/s";

	if (!svg_start(&svg, buf, i, unit)) {
		fprintf(svg.f, "<!-- memcpy -->\n");
		svg_graph(&svg, "red", results[0], mul);
		fprintf(svg.f, "<!-- encode -->\n");
		svg_graph(&svg, "green", results[1], mul);
		fprintf(svg.f, "<!-- decode -->\n");
		svg_graph(&svg, "blue", results[2], mul);
		svg_end(&svg);
	}
	free(buf);
}

int main(int argc, char **argv) {
	size_t i, j, n = 100, n1, n2;
	uint8_t *buf, *out;
	unsigned nrep = 5, nlimit = 100;
	int64_t t0, t1;
	float results[3][21];
	const char *svg_name = NULL;
	float svg_max = 0;

	randseed = time(NULL);

	while (argc > 1) {
		if (argc > 2 && !strcmp(argv[1], "-m")) {
			n = atoi(argv[2]);
			if (!n || n > 877) return 1; /* >= 2GB */
			argc -= 2; argv += 2;
		} else if (argc > 2 && !strcmp(argv[1], "-r")) {
			nrep = atoi(argv[2]);
			if (nrep - 1 >= 1000) return 1;
			argc -= 2; argv += 2;
		} else if (argc > 2 && !strcmp(argv[1], "--seed")) {
			randseed = atoi(argv[2]);
			argc -= 2; argv += 2;
		} else if (argc > 2 && !strcmp(argv[1], "--limit")) {
			nlimit = atol(argv[2]);
			if (nlimit - 1 >= 10000) return 1;
			argc -= 2; argv += 2;
		} else if (argc > 2 && !strcmp(argv[1], "--svg")) {
			svg_name = argv[2];
			argc -= 2; argv += 2;
		} else if (argc > 2 && !strcmp(argv[1], "--svg-max")) {
			svg_max = atof(argv[2]);
			argc -= 2; argv += 2;
		} else break;
	}

#ifdef TB32_BENCH
	printf("TB64: %s\n", TB32_NAME); 
#else
	printf("vector: "
#if CRZY64_VEC && CRZY64_NEON && defined(__aarch64__)
		"neon-arm64"
#elif CRZY64_VEC && CRZY64_NEON
		"neon"
#elif CRZY64_VEC && defined(__AVX2__)
		"avx2"
#elif CRZY64_VEC && defined(__SSE4_1__)
		"sse4.1"
#elif CRZY64_VEC && defined(__SSSE3__)
		"ssse3"
#elif CRZY64_VEC && defined(__SSE2__)
		"sse2"
#else
		"none"
#endif
#if CRZY64_FAST64
		", fast64: yes"
#else
		", fast64: no"
#endif
#if CRZY64_UNALIGNED
		", unaligned: yes"
#else
		", unaligned: no"
#endif
		"\n");
#endif
	printf("size: %u MB, repeat: %u, limit: %u\n\n", (int)n, nrep, nlimit);

	n1 = n << 20;
	n2 = (n1 * 4 + 2) / 3;
	nlimit = n1 / nlimit;
	if (nlimit < 1) nlimit = 1;

	if (!(buf = (uint8_t*)malloc(n1 + n2 + 1))) return 1;
	out = buf + n1;
	for (i = 0; i < n1 + n2; i++) buf[i] = i ^ 0x55;

#define BENCH(name, code) \
	for (t1 = i = 0; i < nrep; i++) { \
		t0 = get_time_usec(); \
		code; \
		t0 = get_time_usec() - t0; \
		if (!i || t0 < t1) t1 = t0; \
	} \
	BENCH_PRINT(name)
#define BENCH_PRINT(name) \
	printf("%s: %.3fms (%.2f MB/s)\n", name, \
			t1 * (1e6 / TIMER_FREQ * 0.001), n1 * (TIMER_FREQ / (1 << 20)) / t1);

	BENCH("memcpy", memcpy(out, buf, n1))
	BENCH("encode", crzy64_encode(out, buf, n1))
	BENCH("decode", crzy64_decode(buf, out, n2))
	BENCH("encode_unaligned", crzy64_encode(out + 1, buf + 1, n1))
	BENCH("decode_unaligned", crzy64_decode(buf + 1, out + 1, n2))

#undef BENCH_PRINT
#define BENCH_PRINT(name) \
	printf("%s (%u%s): %.2f MB/s\n", name, \
			1 << j % 10, j < 10 ? "" : j < 20 ? "K" : "M", \
			res = n * (TIMER_FREQ / (1 << 20)) / t1);

#define BENCH_BLOCK(name) \
	for (j = 0; j <= 20; j++) { \
		size_t n3 = 1 << j, n4 = (n3 * 4 + 2) / 3; \
		size_t n5 = n1 / n3, n6 = n2 / n4, nn; \
		double res; \
		n6 = n5 < n6 ? n5 : n6; \
		/* very slow, need limit */ \
		n5 = n5 < nlimit ? n5 : nlimit; \
		nn = n = n5 * n3; \
		BENCH("memcpy", BLOCK(memcpy, out, buf, n3, n3)) \
		results[0][j] = res; \
		n5 = n6 < nlimit ? n6 : nlimit; \
		nn = n = n5 * n3; \
		BENCH("encode", BLOCK(crzy64_encode, out, buf, n3, n4)) \
		results[1][j] = res; \
		nn = n5 * n4; \
		BENCH("decode", BLOCK(crzy64_decode, buf, out, n4, n3)) \
		results[2][j] = res; \
	} \
	if (svg_name) \
		write_svg(svg_name, "_" name ".svg", results, svg_max);

#define BLOCK(fn, d, s, n3, n4) do { \
	size_t k = nn; \
	while (k) fn(d, s, n3), k -= n3; \
} while (0)

	printf("\nblock repeat (cached):\n");
	BENCH_BLOCK("cached")

#undef BLOCK
#define BLOCK(fn, d, s, n3, n4) do { \
	size_t k = 0, x = 0; \
	while (k < nn) fn(d + x, s + k, n3), k += n3, x += n4; \
} while (0)

	printf("\nblock repeat (consecutive):\n");
	BENCH_BLOCK("forward")

#undef BLOCK
#define BLOCK(fn, d, s, n3, n4) do { \
	size_t k = nn, x = n5 * n4; \
	while (k) k -= n3, x -= n4, fn(d + x, s + k, n3); \
} while (0)

	printf("\nblock repeat (backwards):\n");
	BENCH_BLOCK("backward")

#undef BLOCK
#define BLOCK(fn, d, s, n3, n4) do { \
	size_t k, x; (void)nn; \
	for (k = 0; k < n5; k++) x = bench_rand(n5), \
		fn(d + x * n4, s + x * n3, n3); \
} while (0)

	printf("\nblock repeat (random order):\n");
	BENCH_BLOCK("random")

	return 0;
}
