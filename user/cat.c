#include <lib.h>

char buf[8192];

void cat(int f, char *s) {
	long n;
	int r;

	while ((n = read(f, buf, (long)sizeof buf)) > 0) {
		if ((r = write(1, buf, n)) != n) {
			return -1;
		}
	}
	// printf("asdasdas\n");
	if (n < 0) {
		return -1;
	}
}

int main(int argc, char **argv) {
	int f, i;

	if (argc == 1) {
		cat(0, "<stdin>");
	} else {
		char *buf[512];
		for (i = 1; i < argc; i++) {
			fr2abs(buf, argv[i]);
			argv[i] = buf;
			// printf("%d")
			f = open(argv[i], O_RDONLY);
			if (f < 0) {
				// user_panic("can't open %s: %d", argv[i], f);
				return -1;
			} else {
				cat(f, argv[i]);
				close(f);
			}
		}
	}
	return 0;
}
