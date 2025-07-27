#include <lib.h>

int flag[256];

int lsdir(char *, char *);
int ls1(char *, u_int, u_int, char *);

int ls(char *path, char *prefix) {
	int r;
	struct Stat st;

	char *buf[512];
	fr2abs(buf, path);
	path = buf;
	// printf("%s\n", path);
	if ((r = stat(path, &st)) < 0) {
		// user_panic("stat %s: %d", path, r);
		return r;
	}
	if (st.st_isdir && !flag['d']) {
		return lsdir(path, prefix);
	} else {
		return ls1(0, st.st_isdir, st.st_size, path);
	}
	return 0;
}

int lsdir(char *path, char *prefix) {
	int fd, n;
	struct File f;
	if ((fd = open(path, O_RDONLY)) < 0) {
		// user_panic("open %s: %d", path, fd);
		return fd;
	}
	while ((n = readn(fd, &f, sizeof f)) == sizeof f) {
		if (f.f_name[0]) {
			try(ls1(prefix, f.f_type == FTYPE_DIR, f.f_size, f.f_name));
		}
	}
	if (n > 0) {
		// user_panic("short read in directory %s", path);
		return -1;
	}
	if (n < 0) {
		// user_panic("error reading directory %s: %d", path, n);
		return -1;
	}
	return 0;
}

int ls1(char *prefix, u_int isdir, u_int size, char *name) {
	char *sep;

	// debugf("asdasdasd\n");
	if (flag['l']) {
		printf("%11d %c ", size, isdir ? 'd' : '-');
	}
	if (prefix) {
		if (prefix[0] && prefix[strlen(prefix) - 1] != '/') {
			sep = "/";
		} else {
			sep = "";
		}
		printf("%s%s", prefix, sep);
	}
	// debugf("asdasdasd1  %s\n", name);
	printf("%s", name);
	// debugf("asdasdasd2  %s\n", name);
	if (flag['F'] && isdir) {
		printf("/");
	}
	printf(" ");
	return 0;
}

int usage(void) {
	printf("usage: ls [-dFl] [file...]\n");
	// exit();
	return -1;
}

int main(int argc, char **argv) {
	int i;
	ARGBEGIN {
	default:
		try(usage());
	case 'd':
	case 'F':
	case 'l':
		flag[(u_char)ARGC()]++;
		break;
	}
	ARGEND

	if (argc == 0) {
		try(ls(".", ""));
	} else {
		for (i = 0; i < argc; i++) {
			try(ls(argv[i], argv[i]));
		}
	}
	printf("\n");
	return 0;
}
