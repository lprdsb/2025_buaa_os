#include <lib.h>

void usage(void) {
	printf("usage: touch file...\n");
	exit();
}

int main(int argc, char *argv[]) {
	int fd;
	if (argc < 2) {
		usage();
	}
	
	char *buf[512];
	for (int i = 1; i < argc; i++) {
		fr2abs(buf, argv[i]);
		// argv[i] = buf;
		// printf("%s\n", argv[i]);
		int fdnum;
		struct Filefd *fd;
		if ((fdnum = open(buf, O_CREAT)) >= 0) {
			close(fdnum);
			int r;
			if ((r = fd_lookup(fdnum, &fd)) < 0) {
				return r;
			}
			fd->f_file.f_type = FTYPE_REG;
		}
		else{
			printf("touch: cannot touch '%s': No such file or directory\n", argv[i]);
			return -1;
		}
	}
	
	return 0;
}