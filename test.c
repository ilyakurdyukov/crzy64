#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "crzy64.h"

#define N 128
#define GUARD_SIZE 8

int main() {
	static const uint8_t set[] = {
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
		"ghijklmnopqrstuvwxyz0123456789"
#if CRZY64_AT
		"@/"
#else
		"./"
#endif
	};
	uint8_t src[N * 3];
	size_t n; unsigned i, j;
	uint8_t valid[256] = { 0 };
	uint8_t buf0[N * 4 + GUARD_SIZE * 2], *buf = buf0 + GUARD_SIZE;
	uint8_t out0[N * 3 + GUARD_SIZE * 2], *out = out0 + GUARD_SIZE;
	uint8_t guard[GUARD_SIZE];

	for (i = 0; i < 64; i++) valid[set[i]] = 1;

	srand(time(NULL));

	for (i = 0; i < sizeof(guard); i++)
		guard[i] = rand();

#define ERR(msg) do { \
	fprintf(stderr, "%s at length %d\n", msg, i); return 1; \
} while (0)
#define CHECK_GUARD(p, x) do { \
	if (memcmp((uint8_t*)(p) + (x) * sizeof(guard), guard, sizeof(guard))) \
		ERR(x < 0 ? "left guard damaged" : "right guard damaged"); \
} while (0)
#define SET_GUARD(p, x) \
	memcpy((uint8_t*)(p) + (x) * sizeof(guard), guard, sizeof(guard))

	SET_GUARD(buf, -1);
	SET_GUARD(out, -1);

	for (i = 1; i <= N; i++) {
		src[i - 1] = rand();
		j = (i * 4 + 2) / 3;
		SET_GUARD(buf + j, 0);
		n = crzy64_encode(buf, src, i);
		if (n != j) ERR("invalid encoded size");
		CHECK_GUARD(buf, -1);
		CHECK_GUARD(buf + j, 0);
		for (j = 0; j < n; j++)
			if (!valid[buf[j]]) ERR("invalid character");
		SET_GUARD(out + i, 0);
		n = crzy64_decode(out, buf, n);
		if (n != i) ERR("invalid decoded size");
		CHECK_GUARD(out, -1);
		CHECK_GUARD(out + i, 0);
		if (memcmp(src, out, i))
			ERR("doesn't match the source");
	}
	return 0;
}
