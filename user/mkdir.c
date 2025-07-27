#include <lib.h>

void usage(void) {
	printf("usage: mkdir dir...\n");
	exit();
}

int flag[256];
int main(int argc, char *argv[]) {

	ARGBEGIN {
		case 'p':
			flag[(u_char)ARGC()]++;
			break;
		}
	ARGEND

	int r;
	int rt = 0;
	struct Stat stat_buf;
	if (argc < 1) {
		usage();
	}

	char *buf[512];
	for (int i = 0; i < argc; i++) {
		fr2abs(buf, argv[i]);
		// argv[i] = buf;
        // printf("%d####%s\n", i, argv[i]);
		if(flag['p']){
			if ((r = open(buf, O_FCREAT)) < 0) {
				return r;
			}
			close(r);
		}
		else{
			if (stat(buf, &stat_buf) >= 0) {
				printf("mkdir: cannot create directory '%s': File exists\n", argv[i]);
				rt = -1;
				continue;
			}
			if ((r = mkdir(buf)) < 0) {
				printf("mkdir: cannot create directory '%s': No such file or directory\n", argv[i]);
				rt = r;
			}
		}
	}

	return rt;
}
