#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crzy64.h"

#define N 4096

int main(int argc, char **argv) {
	uint8_t buf[N * 4], out[N * 4];
	size_t n;

	if (argc > 1 && !strcmp(argv[1], "-d")) {
		do {
			n = fread(buf, 1, N * 4, stdin);
			if (!n) break;
			fwrite(out, 1, crzy64_decode(out, buf, n), stdout);
		} while (n == N * 4);
	} else {
		do {
			n = fread(buf, 1, N * 3, stdin);
			if (!n) break;
			fwrite(out, 1, crzy64_encode(out, buf, n), stdout);
		} while (n == N * 3);
	}
}
