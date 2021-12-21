#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "crzy64.h"

#define N 64

int main() {
	uint8_t src[N * 3], buf[N * 4], out[N * 3];
	size_t n; unsigned i;

	srand(time(NULL));

	for (i = 1; i <= N; i++) {
		src[i - 1] = rand();
		n = crzy64_encode(buf, src, i);
		n = crzy64_decode(out, buf, n);
		if (n != i || memcmp(src, out, i)) {
			fprintf(stderr, "error at length %d\n", i);
			return 1;
		}
	}
}
