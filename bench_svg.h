
typedef struct {
	int nx, ny;
	int x0, x1, xm;
	int y0, y1, ym;
	int w, h;
	FILE *f;
} svg_writer_t;

static int svg_start(svg_writer_t *s, const char *fn, float mul, const char *unit) {
	FILE *f; int i;
	s->w = s->x0 + s->nx * s->xm + s->x1;
	s->h = s->y0 + s->ny * s->ym + s->y1;
	s->f = f = fopen(fn, "wb");
	if (!f) return 1;

	fprintf(f, 
"<svg xmlns='http://www.w3.org/2000/svg' width='%d' height='%d'>\n"
"<style>\n"
"path { fill: transparent; fill-opacity: 0; }\n"
"text { alignment-baseline: middle; text-anchor: middle; }\n"
"</style>\n"
"<rect width='100%%' height='100%%' fill='white'/>\n", s->w, s->h);

	for (i = 0; i <= s->ny; i++) {
		int y = s->h - s->y0 - i * s->ym;
		double l = i * mul;
		fprintf(f, "<path stroke='gray' d='M %d %d h %d'/>\n",
				s->x0, y, s->nx * s->xm);
		fprintf(f, "<text x='%d' y='%d'>%g%s</text>\n",
				s->x0 / 2, y, l, unit);
	}

	for (i = 0; i <= s->nx; i += 2) {
		fprintf(f, "<text x='%d' y='%d'>%d%s</text>\n",
				s->x0 + i * s->xm, s->h - s->y0 / 2,
				1 << i % 10, i < 10 ? "" : i < 20 ? "K" : "M");
	}
	return 0;
}

static void svg_graph(svg_writer_t *s, const char *clr,
		float *arr, float mul) {
	int i;
	if (!s->f) return;
	mul *= s->ym;

	fprintf(s->f, "<g fill='%s'>\n", clr);
	for (i = 0; i <= s->nx; i++) {
		double y = s->h - s->y0 - arr[i] * mul;
		fprintf(s->f, "<circle cx='%d' cy='%.2f' r='4'/>\n",
				s->x0 + i * s->xm, y);
	}
	fprintf(s->f, "</g>\n");

	fprintf(s->f, "<path stroke='%s' stroke-width='2' d='", clr);
	for (i = 0; i <= s->nx; i++) {
		double y = s->h - s->y0 - arr[i] * mul;
		fprintf(s->f, "%s %d %.2f",
				i ? " L" : "M", s->x0 + i * s->xm, y);
	}
	fprintf(s->f, "'/>\n");
}

static void svg_end(svg_writer_t *s) {
	if (s->f) {
		fprintf(s->f, "</svg>");
		fclose(s->f);
		s->f = NULL;
	}
}

