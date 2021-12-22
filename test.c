#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "crzy64.h"

#define N 64

int main() {
	static const uint8_t set[64] = {
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
		"ghijklmnopqrstuvwxyz0123456789./"
	};
	uint8_t src[N * 3], buf[N * 4], out[N * 3];
	size_t n; unsigned i, j;
	uint8_t valid[256] = { 0 };

	for (i = 0; i < 64; i++) valid[set[i]] = 1;

	srand(time(NULL));

#define ERR(...) do { \
	fprintf(stderr, __VA_ARGS__); return 1; \
} while (0)

	for (i = 1; i <= N; i++) {
		src[i - 1] = rand();
		n = crzy64_encode(buf, src, i);
		if (n != (i * 4 + 2) / 3)
			ERR("invalid encoded size at length %d\n", i);
		for (j = 0; j < n; j++)
			if (!valid[buf[j]])
				ERR("invaid character at length %d\n", i);
		n = crzy64_decode(out, buf, n);
		if (n != i)
			ERR("invalid decoded size at length %d\n", i);
		if (memcmp(src, out, i))
			ERR("doesn't match the source at length %d\n", i);
	}
}
